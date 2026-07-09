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
        rn, rp, precs, recs = [], [], [], []
        for m in mixes:
            # common mix set: the experiment AND both baselines must be healthy,
            # so vs-nopf and vs-pythia columns cover identical mixes
            if ((m, e) in healthy and (m, "c2_nopf") in healthy
                    and (m, "c2_pythia") in healthy):
                mp, r = healthy[(m, e)]
                rn.append(mp / healthy[(m, "c2_nopf")][0])
                rp.append(mp / healthy[(m, "c2_pythia")][0])
                try:
                    precs.append(float(r["precision"]))
                    recs.append(float(r["recall"]))
                except ValueError:
                    pass
        if not rn:
            continue
        gm = lambda v: math.exp(sum(math.log(x) for x in v) / len(v))
        out.append({
            "exp": e, "n_mixes": len(rn),
            "speedup_vs_nopf": round(gm(rn), 4),
            "speedup_vs_pythia": round(gm(rp), 4),
            "mean_precision": round(sum(precs) / len(precs), 1) if precs else "",
            "mean_recall": round(sum(recs) / len(recs), 1) if recs else "",
        })
    out.sort(key=lambda x: -x["speedup_vs_nopf"])
    with open(outp, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(out[0].keys()))
        w.writeheader()
        [w.writerow(o) for o in out]
    for o in out:
        print(o)

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
