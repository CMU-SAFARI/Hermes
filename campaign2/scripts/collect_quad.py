#!/usr/bin/env python3
"""Campaign-2 scorer: geomean-IPC speedup vs the per-mix nopf baseline.

Usage: collect_quad.py <stats_health.csv> <out.csv>
Input rows: TraceName (trace_mix_N), ExpName, ipc0..ipc3, precision, recall,
Filter. A mix contributes to an experiment's score only if BOTH that
experiment's row and the c2_nopf row for the same mix are healthy
(Filter==1, all four IPCs > 0). Output: one row per experiment with
n_mixes, speedup (geomean over mixes of geomean-IPC ratios), mean pooled
precision/recall (diagnostics).
"""
import csv, math, sys
from collections import defaultdict

def mean_perf(r):
    v = [float(r[f"ipc{i}"]) for i in range(4)]
    if any(x <= 0 for x in v):
        return None
    return math.exp(sum(math.log(x) for x in v) / 4)

def main(inp, outp):
    rows = list(csv.DictReader(open(inp)))
    healthy = {}
    for r in rows:
        if r.get("Filter", "1") != "1":
            continue
        mp = mean_perf(r)
        if mp is None:
            continue
        healthy[(r["TraceName"], r["ExpName"])] = (mp, r)
    exps = sorted({e for (_, e) in healthy})
    mixes = sorted({m for (m, _) in healthy})
    out = []
    for e in exps:
        if e == "c2_nopf":
            continue
        ratios, precs, recs = [], [], []
        for m in mixes:
            if (m, e) in healthy and (m, "c2_nopf") in healthy:
                mp, r = healthy[(m, e)]
                base, _ = healthy[(m, "c2_nopf")]
                ratios.append(mp / base)
                try:
                    precs.append(float(r["precision"]))
                    recs.append(float(r["recall"]))
                except ValueError:
                    pass
        if not ratios:
            continue
        sp = math.exp(sum(math.log(x) for x in ratios) / len(ratios))
        out.append({
            "exp": e, "n_mixes": len(ratios), "speedup": round(sp, 4),
            "mean_precision": round(sum(precs) / len(precs), 1) if precs else "",
            "mean_recall": round(sum(recs) / len(recs), 1) if recs else "",
        })
    out.sort(key=lambda x: -x["speedup"])
    with open(outp, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(out[0].keys()))
        w.writeheader()
        [w.writerow(o) for o in out]
    for o in out:
        print(o)

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
