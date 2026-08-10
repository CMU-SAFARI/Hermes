#!/usr/bin/env python3
"""Estimate DRAM energy for ChampSim runs from their activity counters.

ChampSim has no power model, but DRAM energy is well approximated from event
counts x per-event energy (the Micron TN-40-07 / TN-41-01 methodology). This
reads one or more ChampSim .out files and reports, per run, the DRAM energy
split into activation, read, write and background+refresh components.

Device parameters (IDD currents, timings, voltages) are NOT in this file --
they live in devices/*.ini, one per DRAM model. This file holds only the
equations. See README.md for the model, counter semantics and caveats.

Usage:
    dram_energy.py run.out [more.out ...]            # table on stdout
    dram_energy.py --csv energy.csv results/*.out    # machine-readable
    dram_energy.py --device ddr5_6400 run.out        # another DRAM model
    dram_energy.py --list-devices
"""

import argparse
import configparser
import csv
import glob
import os
import re
import sys

DEVICE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "devices")
DEFAULT_DEVICE = "micron_mt40a1g8_ddr4_3200"


class Device:
    """Device parameters loaded from a devices/*.ini profile."""

    def __init__(self, path):
        cp = configparser.ConfigParser(inline_comment_prefixes=("#", ";"))
        if not cp.read(path):
            raise SystemExit(f"cannot read device profile: {path}")
        self.path = path
        try:
            self.name = cp.get("device", "name", fallback=os.path.basename(path))
            self.verified = cp.getboolean("device", "verified", fallback=False)
            o = cp["organization"]
            self.devices_per_rank = o.getint("devices_per_rank")
            self.burst_transfers = o.getint("burst_transfers")
            self.line_bits = o.getint("line_bits")
            self.data_rate_mtps = o.getint("data_rate_mtps")
            self.vdd = cp.getfloat("voltage_v", "vdd")
            self.vpp = cp.getfloat("voltage_v", "vpp")
            c = cp["current_ma"]
            for k in ("idd0", "idd2n", "idd3n", "idd4r", "idd4w", "idd5b", "ipp0"):
                setattr(self, k, c.getfloat(k))
            t = cp["timing_ns"]
            for k in ("tras", "trc", "trfc", "trefi"):
                setattr(self, k, t.getfloat(k))
            self.io_read_pj = cp.getfloat("io_pj_per_bit", "read")
            self.io_write_pj = cp.getfloat("io_pj_per_bit", "write")
        except (configparser.Error, KeyError, TypeError, ValueError) as exc:
            raise SystemExit(f"malformed device profile {path}: {exc}")

    def energy_per_event(self, mtps):
        """Per-rank energy (J) for one ACT+PRE, one read burst, one write burst,
        and the steady background+refresh power (W), at data rate `mtps`."""
        n = self.devices_per_rank
        # ACT+PRE: IDD0 averaged over tRC, minus the background it already
        # includes (IDD3N while the row is active, IDD2N for the rest).
        # Micron TN-41-01 Eq. 10.
        e_act = n * (self.vdd * (self.idd0 * self.trc
                                 - (self.idd3n * self.tras
                                    + self.idd2n * (self.trc - self.tras)))
                     + self.vpp * self.ipp0 * self.trc) * 1e-12
        t_burst = self.burst_transfers / (mtps * 1e6) * 1e9          # ns
        e_rd = n * self.vdd * (self.idd4r - self.idd3n) * t_burst * 1e-12
        e_wr = n * self.vdd * (self.idd4w - self.idd3n) * t_burst * 1e-12
        e_rd += self.io_read_pj * 1e-12 * self.line_bits
        e_wr += self.io_write_pj * 1e-12 * self.line_bits
        p_bg = n * self.vdd * self.idd3n * 1e-3                       # W
        p_ref = n * self.vdd * (self.idd5b - self.idd3n) * self.trfc / self.trefi * 1e-3
        return e_act, e_rd, e_wr, p_bg + p_ref


def available_devices():
    return sorted(os.path.splitext(os.path.basename(p))[0]
                  for p in glob.glob(os.path.join(DEVICE_DIR, "*.ini")))


def resolve_device(spec):
    """Accept a bare profile name (looked up in devices/) or a path."""
    if os.path.sep in spec or spec.endswith(".ini"):
        return spec
    path = os.path.join(DEVICE_DIR, spec + ".ini")
    if not os.path.exists(path):
        raise SystemExit(f"unknown device '{spec}'. Available: "
                         + ", ".join(available_devices()))
    return path


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


def compute(c, dev, cpu_freq_mhz):
    mtps = c["mtps"] or dev.data_rate_mtps
    e_act1, e_rd1, e_wr1, p_bg = dev.energy_per_event(mtps)
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
    ap.add_argument("out_files", nargs="*", help="ChampSim .out file(s)")
    ap.add_argument("--csv", metavar="PATH", help="write results as CSV")
    ap.add_argument("--device", default=DEFAULT_DEVICE,
                    help=f"DRAM model profile name or path (default {DEFAULT_DEVICE})")
    ap.add_argument("--list-devices", action="store_true",
                    help="list the available device profiles and exit")
    ap.add_argument("--cpu-freq-mhz", type=float, default=4000.0,
                    help="core clock used to turn cycles into seconds (default 4000)")
    args = ap.parse_args()

    if args.list_devices:
        for name in available_devices():
            print(f"  {name:40s} {Device(resolve_device(name)).name}")
        return
    if not args.out_files:
        ap.error("no .out files given")

    dev = Device(resolve_device(args.device))
    if not dev.verified:
        print(f"NOTE: device profile '{dev.name}' is marked unverified — its "
              f"parameters are representative, not transcribed from a datasheet. "
              f"Comparisons are sound; absolute Joules carry ~10-20%.",
              file=sys.stderr)

    rows, skipped = [], []
    for path in args.out_files:
        c = parse(path)
        if c is None:
            skipped.append(path)
            continue
        r = compute(c, dev, args.cpu_freq_mhz)
        r["run"] = os.path.basename(path)[:-4] if path.endswith(".out") else os.path.basename(path)
        rows.append(r)
        if r["mtps"] != dev.data_rate_mtps:
            print(f"WARNING: {r['run']}: dram_io_freq={r['mtps']} MT/s but profile "
                  f"'{dev.name}' describes {dev.data_rate_mtps} MT/s; only the burst "
                  f"time is rescaled (see README caveats).", file=sys.stderr)
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
        print(f"wrote {args.csv} ({len(rows)} run(s), device: {dev.name})")
    else:
        print(f"device: {dev.name}")
        print(f"{'run':<44} {'ACTs':>12} {'reads':>12} {'E_act':>9} {'E_rd':>9} "
              f"{'E_bg':>9} {'E_total':>10}")
        for r in rows:
            print(f"{r['run'][:44]:<44} {r['acts']:12,} {r['reads']:12,} "
                  f"{r['e_act_J']:8.3f}J {r['e_rd_J']:8.3f}J {r['e_bg_J']:8.3f}J "
                  f"{r['e_total_J']:9.3f}J")


if __name__ == "__main__":
    main()
