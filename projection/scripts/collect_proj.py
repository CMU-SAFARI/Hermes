#!/usr/bin/env python3
"""Projection-run scorer with per-workload weighted rollup (owner rubric).

Usage: collect_proj.py <tlist.yml> [<tlist2.yml> ...] <stats_health.csv> <out_prefix>

Rubric: per-trace speedup = ipc / ipc(proj_nopf|proj_pythia, same trace).
Per workload x experiment: WEIGHTED GEOMETRIC mean of trace speedups,
exp(sum(w*ln s)/sum(w)); precision/recall: WEIGHTED ARITHMETIC mean,
sum(w*x)/sum(w). Suite level: unweighted geomean of per-workload speedups;
unweighted arithmetic mean of per-workload P/R (each workload counts once).
A trace is eligible only if its exp row AND both baseline rows are healthy.
Outputs: <out_prefix>_workloads.csv and <out_prefix>_suite.csv.
"""
import csv, math, sys, yaml

def main(argv):
    *tlists, stats, prefix = argv
    meta = {}
    for tl in tlists:
        d = yaml.safe_load(open(tl))
        for e in d[next(iter(d))]:
            n = next(iter(e))
            meta[n] = (e[n]["workload"], float(e[n]["weight"]))
    rows = list(csv.DictReader(open(stats)))
    healthy = {}
    for r in rows:
        if r.get("Filter", "1") != "1":
            continue
        try:
            ipc = float(r["ipc"])
        except (KeyError, ValueError):
            continue
        if ipc <= 0:
            continue
        healthy[(r["TraceName"], r["ExpName"])] = r
    exps = sorted({e for (_, e) in healthy if e != "proj_nopf"})
    wl_rows, suite_rows = [], []
    for e in exps:
        per_wl = {}
        for (t, ee) in list(healthy):
            if ee != e or t not in meta:
                continue
            if (t, "proj_nopf") not in healthy or (t, "proj_pythia") not in healthy:
                continue
            wl, w = meta[t]
            r = healthy[(t, e)]
            ipc = float(r["ipc"])
            sn = ipc / float(healthy[(t, "proj_nopf")]["ipc"])
            sp = ipc / float(healthy[(t, "proj_pythia")]["ipc"])
            try:
                pr = (float(r["precision"]), float(r["recall"]))
            except (KeyError, ValueError):
                pr = None
            per_wl.setdefault(wl, []).append((w, sn, sp, pr))
        wl_out = {}
        for wl, items in sorted(per_wl.items()):
            W = sum(w for w, *_ in items)
            gn = math.exp(sum(w * math.log(sn) for w, sn, _, _ in items) / W)
            gp = math.exp(sum(w * math.log(sp) for w, _, sp, _ in items) / W)
            prs = [(w, pr) for w, _, _, pr in items if pr]
            if prs:
                Wp = sum(w for w, _ in prs)
                ap = sum(w * pr[0] for w, pr in prs) / Wp
                ar = sum(w * pr[1] for w, pr in prs) / Wp
            else:
                ap = ar = None
            wl_out[wl] = (len(items), gn, gp, ap, ar)
            wl_rows.append({"exp": e, "workload": wl, "n_traces": len(items),
                            "speedup_vs_nopf": round(gn, 4), "speedup_vs_pythia": round(gp, 4),
                            "precision": round(ap, 2) if ap is not None else "",
                            "recall": round(ar, 2) if ar is not None else ""})
        if wl_out:
            vals = list(wl_out.values())
            gm = lambda xs: math.exp(sum(math.log(x) for x in xs) / len(xs))
            pv = [v[3] for v in vals if v[3] is not None]
            rv = [v[4] for v in vals if v[4] is not None]
            suite_rows.append({"exp": e, "n_workloads": len(vals),
                               "speedup_vs_nopf": round(gm([v[1] for v in vals]), 4),
                               "speedup_vs_pythia": round(gm([v[2] for v in vals]), 4),
                               "precision": round(sum(pv) / len(pv), 2) if pv else "",
                               "recall": round(sum(rv) / len(rv), 2) if rv else ""})
    for name, data in [("workloads", wl_rows), ("suite", suite_rows)]:
        with open(f"{prefix}_{name}.csv", "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(data[0].keys()))
            w.writeheader()
            [w.writerow(r) for r in data]
    for r in suite_rows:
        print(r)

if __name__ == "__main__":
    main(sys.argv[1:])
