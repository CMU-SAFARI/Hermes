#!/usr/bin/env python3
"""Estimate DRAM energy for ChampSim runs from their activity counters.

ChampSim has no power model, but DRAM energy is well approximated from event
counts x per-event energy (the Micron TN-40-07 / TN-41-01 methodology). This
reads one or more ChampSim .out files and reports, per run, the DRAM energy
split into activation, read, write and background+refresh components.

See README.md for the model, the counter semantics and the caveats.

Usage:
    dram_energy.py run.out [more.out ...]            # table on stdout
    dram_energy.py --csv energy.csv results/*.out    # machine-readable
"""

import argparse
import csv
import os
import re
import sys

# --- DDR4-3200 device profile (Micron 8Gb x8; a rank is 8 such devices) -----
# IDD/IPP are per device in mA, timings in ns, voltages in V. These are
# representative datasheet values -- see README.md "Caveats" before quoting
# absolute Joules.
VDD, VPP = 1.2, 2.5
IDD0, IDD2N, IDD3N = 65.0, 34.0, 52.0
IDD4R, IDD4W, IDD5B = 200.0, 160.0, 175.0
IPP0 = 3.0
tRAS, tRC = 32.0, 45.75
tRFC, tREFI = 350.0, 7800.0
DEVICES_PER_RANK = 8
PROFILE_MTPS = 3200          # data rate the timings/IDDs above describe
BURST_TRANSFERS = 8          # BL8
LINE_BITS = 512              # 64B cache line

# I/O + termination, NOT from the Micron note -- a literature approximation.
IO_READ_PJ_PER_BIT = 4.0
IO_WRITE_PJ_PER_BIT = 6.0


def energy_per_event(mtps):
    """Per-rank energy (J) for one ACT+PRE, one read burst, one write burst,
    and the steady background+refresh power (W), at data rate `mtps`."""
    n = DEVICES_PER_RANK
    # ACT+PRE: IDD0 averaged over tRC, minus the background it already includes
    # (IDD3N while the row is active, IDD2N for the rest). Micron TN-41-01 Eq.10.
    e_act = n * (VDD * (IDD0 * tRC - (IDD3N * tRAS + IDD2N * (tRC - tRAS)))
                 + VPP * IPP0 * tRC) * 1e-12
    t_burst = BURST_TRANSFERS / (mtps * 1e6) * 1e9          # ns
    e_rd = n * VDD * (IDD4R - IDD3N) * t_burst * 1e-12
    e_wr = n * VDD * (IDD4W - IDD3N) * t_burst * 1e-12
    e_rd += IO_READ_PJ_PER_BIT * 1e-12 * LINE_BITS
    e_wr += IO_WRITE_PJ_PER_BIT * 1e-12 * LINE_BITS
    p_bg = n * VDD * IDD3N * 1e-3                            # W
    p_ref = n * VDD * (IDD5B - IDD3N) * tRFC / tREFI * 1e-3  # W
    return e_act, e_rd, e_wr, p_bg + p_ref


RE_CH = re.compile(r"^Channel_(\d+)_(RQ|WQ)_row_buffer_(hit|miss)\s+(\d+)")
RE_ROWOPEN = re.compile(r"^DRAM_DDRP_row_open_act\s+(\d+)")
RE_ROWOPEN_KNOB = re.compile(r"^ddrp_row_open\s+(\d+)")
RE_IOFREQ = re.compile(r"^dram_io_freq\s+(\d+)")
RE_FINISHED = re.compile(r"^Finished CPU \d+ instructions:\s+\d+\s+cycles:\s+(\d+)")


def parse(path):
    """Pull the DRAM counters out of one ChampSim .out. Returns None if the
    run did not finish (deadlock/timeout), so callers can skip it."""
    c = {"rq_hit": 0, "rq_miss": 0, "wq_hit": 0, "wq_miss": 0,
         "row_open_act": 0, "cycles": None, "mtps": None,
         "has_act_counter": False, "row_open_enabled": False}
    with open(path, errors="replace") as f:
        for line in f:
            m = RE_CH.match(line)
            if m:
                _, q, kind, val = m.groups()
                c[f"{q.lower()}_{kind}"] += int(val)
                continue
            m = RE_ROWOPEN.match(line)
            if m:
                c["row_open_act"] = int(m.group(1))
                c["has_act_counter"] = True
                continue
            m = RE_ROWOPEN_KNOB.match(line)
            if m:
                c["row_open_enabled"] = m.group(1) == "1"
                continue
            m = RE_IOFREQ.match(line)
            if m:
                c["mtps"] = int(m.group(1))
                continue
            m = RE_FINISHED.match(line)
            if m:
                c["cycles"] = int(m.group(1))
    return c if c["cycles"] else None


def compute(c, cpu_freq_mhz):
    mtps = c["mtps"] or PROFILE_MTPS
    e_act1, e_rd1, e_wr1, p_bg = energy_per_event(mtps)
    # Reads/writes are real column accesses. Row-opens move no data and are
    # excluded from the RQ counters by construction, so their activations come
    # from the dedicated counter instead.
    reads = c["rq_hit"] + c["rq_miss"]
    writes = c["wq_hit"] + c["wq_miss"]
    acts = c["rq_miss"] + c["wq_miss"] + c["row_open_act"]
    seconds = c["cycles"] / (cpu_freq_mhz * 1e6)
    e = {"acts": acts, "reads": reads, "writes": writes,
         "row_open_act": c["row_open_act"], "cycles": c["cycles"],
         "mtps": mtps, "seconds": seconds,
         "stale_act_counter": c["row_open_enabled"] and not c["has_act_counter"],
         "e_act_J": acts * e_act1, "e_rd_J": reads * e_rd1,
         "e_wr_J": writes * e_wr1, "e_bg_J": seconds * p_bg}
    e["e_total_J"] = e["e_act_J"] + e["e_rd_J"] + e["e_wr_J"] + e["e_bg_J"]
    return e


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out_files", nargs="+", help="ChampSim .out file(s)")
    ap.add_argument("--csv", metavar="PATH", help="write results as CSV")
    ap.add_argument("--cpu-freq-mhz", type=float, default=4000.0,
                    help="core clock used to turn cycles into seconds (default 4000)")
    args = ap.parse_args()

    rows, skipped = [], []
    for path in args.out_files:
        c = parse(path)
        if c is None:
            skipped.append(path)
            continue
        r = compute(c, args.cpu_freq_mhz)
        r["run"] = os.path.basename(path)[:-4] if path.endswith(".out") else os.path.basename(path)
        rows.append(r)
        if r["mtps"] != PROFILE_MTPS:
            print(f"WARNING: {r['run']}: dram_io_freq={r['mtps']} MT/s but the device "
                  f"profile describes DDR4-{PROFILE_MTPS}; only the burst time is "
                  f"rescaled (see README caveats).", file=sys.stderr)
        if r["stale_act_counter"]:
            print(f"WARNING: {r['run']}: run uses --ddrp_row_open but has no "
                  f"DRAM_DDRP_row_open_act counter, so its row activations are "
                  f"MISSING and the energy below is a large UNDERCOUNT. Re-run with "
                  f"a build that has the counter.", file=sys.stderr)

    if skipped:
        print(f"skipped {len(skipped)} unfinished run(s) (no 'Finished CPU' line): "
              + ", ".join(os.path.basename(p) for p in skipped[:5])
              + (" ..." if len(skipped) > 5 else ""), file=sys.stderr)
    if not rows:
        sys.exit("no finished runs to report")

    cols = ["run", "cycles", "seconds", "acts", "reads", "writes", "row_open_act",
            "e_act_J", "e_rd_J", "e_wr_J", "e_bg_J", "e_total_J"]
    if args.csv:
        with open(args.csv, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=cols, extrasaction="ignore")
            w.writeheader()
            w.writerows(rows)
        print(f"wrote {args.csv} ({len(rows)} run(s))")
    else:
        print(f"{'run':<44} {'ACTs':>12} {'reads':>12} {'E_act':>9} {'E_rd':>9} "
              f"{'E_bg':>9} {'E_total':>10}")
        for r in rows:
            print(f"{r['run'][:44]:<44} {r['acts']:12,} {r['reads']:12,} "
                  f"{r['e_act_J']:8.3f}J {r['e_rd_J']:8.3f}J {r['e_bg_J']:8.3f}J "
                  f"{r['e_total_J']:9.3f}J")


if __name__ == "__main__":
    main()
