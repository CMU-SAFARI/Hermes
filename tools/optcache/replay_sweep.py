#!/usr/bin/env python3
"""Replay captured LLC access traces through OPT at every capacity.

One capture per benchmark feeds all six capacities: the access stream arriving
at the LLC is capacity-independent (measured -- Core_0_LLC_total_access moved by
3 accesses out of 2,420,052 across a 32x range), only the hit/miss outcome
changes, and OptCache recomputes that itself.

Usage:
  replay_sweep.py --dumps <llcdump dir> [...] --stats-dir <batch dir> [...] \
                  --out results.csv [--jobs N]

Dumps stay on node-local disk and are never copied to NFS, so this runs pinned
to the node holding them; --stats-dir points at the NFS batch dir with the
capture runs' <tag>.out files, which the validation gate needs.

Emits one row per (trace, capacity, bypass). `gate_ok` is the validation gate:
OptCache.trace.total replays the online policy's own outcome from the trace's
hit bit, so it must equal that run's Core_0_LLC_total_access. It is meaningful
only at the capacity the trace was CAPTURED at (2MB); elsewhere trace.* still
describes the 2MB run, which is why opt_miss must be compared against the online
sweep's miss count at the matching capacity, never against trace_miss.
"""
import argparse
import csv
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor
from functools import partial

# LLC_SET from inc/uarch/custom_*.h (NUM_CPUS=1); LLC_WAY is 16 throughout.
CAPACITIES = [("256KB", 256), ("512KB", 512), ("1MB", 1024), ("2MB", 2048),
              ("4MB", 4096), ("8MB", 8192), ("16MB", 16384), ("32MB", 32768)]
WAYS = 16
HERE = os.path.dirname(os.path.abspath(__file__))


def counters(text):
    return {m.group(1): int(m.group(2))
            for m in re.finditer(r"^(\S+)\s+(\d+)\s*$", text, re.M)}


# The gate is documented to hold to a tiny fraction, NOT exactly: the trace
# counts an access on arrival, sim_access counts it on fill, and the few in
# flight at the ROI edge fall in that gap. Measured across 59 SPECrate traces
# the worst was 0.00046% (7 accesses in 1.5M). A real fault -- wrong trace/reuse
# pairing, a truncated dump -- misses by percent, so this separates them.
GATE_TOL_PCT = 0.01


def gate_delta(c, on):
    o = on.get("Core_0_LLC_total_access")
    t = c.get("OptCache.trace.total")
    if not o or t is None:
        return None
    return round((t - o) / o * 100, 6)


def gate_ok(c, on):
    d = gate_delta(c, on)
    return None if d is None else abs(d) <= GATE_TOL_PCT


def online_stats(dump_path, tag, stats_dirs):
    """Counters from the capture run that produced this dump, read from
    <stats-dir>/<tag>.out. Dumps live on node-local disk and never reach NFS,
    so the batch dir holding the .out files is passed in separately; the
    sibling-of-the-dump layout is kept as a fallback."""
    cands = [os.path.join(d, tag + ".out") for d in stats_dirs]
    cands.append(os.path.join(os.path.dirname(os.path.dirname(dump_path)),
                              tag + ".out"))
    for out in cands:
        if os.path.exists(out):
            with open(out, errors="replace") as f:
                return counters(f.read())
    return {}


def one_trace(dump_path, stats_dirs=()):
    tag = os.path.basename(dump_path)[:-len(".zst")]
    on = online_stats(dump_path, tag, stats_dirs)
    rows = []
    with tempfile.TemporaryDirectory() as td:
        base = os.path.join(td, "r")
        rc = subprocess.run([os.path.join(HERE, "gen_fwd_reuse"), dump_path, base],
                            capture_output=True, text=True)
        if rc.returncode != 0:
            # A truncated dump exits 1 here rather than replaying short.
            return [], f"{tag}: gen_fwd_reuse failed rc={rc.returncode}: {rc.stderr.strip()[:200]}"
        reuse = base + ".reuse.zst"
        for name, sets in CAPACITIES:
            for byp in (0, 1):
                p = subprocess.run(
                    [os.path.join(HERE, "optcache_driver"), str(sets), str(WAYS),
                     dump_path, reuse, str(byp), "--roi-marker"],
                    capture_output=True, text=True)
                if p.returncode != 0:
                    return [], f"{tag}: driver failed at {name}/bypass={byp}: {p.stderr.strip()[:200]}"
                c = counters(p.stdout)
                rows.append({
                    "trace": tag.replace("_lru_capture", ""),
                    "capacity": name, "sets": sets, "ways": WAYS, "bypass": byp,
                    "opt_miss": c.get("OptCache.access.miss"),
                    "opt_hit": c.get("OptCache.access.hit"),
                    "opt_total": c.get("OptCache.access.total"),
                    # load_* is the study's headline metric: total misses are
                    # dominated by writebacks and rank policies the other way.
                    "opt_load_miss": c.get("OptCache.access.LOAD.miss"),
                    "opt_load_total": c.get("OptCache.access.LOAD.total"),
                    "bypasses": c.get("OptCache.cache.bypass"),
                    "trace_total": c.get("OptCache.trace.total"),
                    "trace_miss": c.get("OptCache.trace.miss"),
                    "trace_load_miss": c.get("OptCache.trace.LOAD.miss"),
                    "online_llc_access": on.get("Core_0_LLC_total_access"),
                    "online_total_miss": on.get("Core_0_LLC_total_miss"),
                    "online_load_miss": on.get("Core_0_LLC_load_miss"),
                    "instructions": on.get("Core_0_total_instructions"),
                    "gate_delta_pct": gate_delta(c, on),
                    # None when the capture's .out is not beside the dump, which
                    # is "not checked", not "mismatch" -- keep them distinct.
                    "gate_ok": gate_ok(c, on),
                })
    return rows, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dumps", nargs="+", required=True)
    ap.add_argument("--stats-dir", nargs="*", default=[],
                    help="batch dir(s) holding <tag>.out from the capture runs; "
                         "needed because dumps stay node-local and the .out files "
                         "do not sit beside them")
    ap.add_argument("--out", required=True)
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    a = ap.parse_args()

    dumps = sorted(os.path.join(d, f) for d in a.dumps
                   for f in os.listdir(d) if f.endswith(".zst"))
    if not dumps:
        sys.exit("no .zst dumps found")
    print(f"{len(dumps)} dump(s), {a.jobs} worker(s)", file=sys.stderr)

    all_rows, errors = [], []
    with ProcessPoolExecutor(max_workers=a.jobs) as ex:
        worker = partial(one_trace, stats_dirs=tuple(a.stats_dir))
        for rows, err in ex.map(worker, dumps):
            if err:
                errors.append(err)
                print("  ERROR " + err, file=sys.stderr)
            else:
                all_rows.extend(rows)
                print(f"  done {rows[0]['trace']}", file=sys.stderr)

    with open(a.out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(all_rows[0].keys()))
        w.writeheader()
        w.writerows(all_rows)

    at2mb = [r for r in all_rows if r["capacity"] == "2MB"]
    bad = [r for r in at2mb if r["gate_ok"] is False]
    unchecked = sorted({r["trace"] for r in at2mb if r["gate_ok"] is None})
    print(f"\nwrote {len(all_rows)} rows to {a.out}", file=sys.stderr)
    print(f"traces ok: {len(all_rows)//(len(CAPACITIES)*2)}  failed: {len(errors)}",
          file=sys.stderr)
    if unchecked:
        print(f"gate NOT CHECKED for {len(unchecked)} trace(s) (no .out beside "
              f"the dump): {', '.join(unchecked[:3])}...", file=sys.stderr)
    if bad:
        names = sorted({r["trace"] for r in bad})
        print(f"VALIDATION GATE FAILED at 2MB for {len(names)} trace(s) "
              f"(tol {GATE_TOL_PCT}%): " + ", ".join(names), file=sys.stderr)
    worst = max((abs(r["gate_delta_pct"]) for r in at2mb
                 if r["gate_delta_pct"] is not None), default=None)
    if worst is not None:
        print(f"worst gate delta at 2MB: {worst:.5f}% (tol {GATE_TOL_PCT}%)",
              file=sys.stderr)
    if errors or bad:
        sys.exit(1)


if __name__ == "__main__":
    main()
