#!/usr/bin/env python3
"""Score one campaign round from the fetched stats.csv.

Usage: collect.py <campaign_dir> <round> <stats_csv>

Reads rounds/roundNN/manifest.json + stats.csv (TraceName,ExpName,ipc,
precision,recall,Filter). Scores only Filter==1 rows. A trace missing a
valid row for ANY tuning-window experiment of the round (or missing a
baseline IPC) is dropped from the round's common trace set, so every config
is scored on the identical trace set.

Writes rounds/roundNN/{rollup.csv,top5.json,report.md} and, in round 1,
<campaign_dir>/baselines.json (per-trace nopf/pythia IPC, tuning window).
Does NOT touch state.json — the beam decision is a judgment step.
"""
import csv
import json
import math
import os
import sys


def geomean(xs):
    return math.exp(sum(math.log(x) for x in xs) / len(xs))


def spearman(a, b):
    """Spearman rank correlation of two equal-length score lists."""
    def ranks(v):
        order = sorted(range(len(v)), key=lambda i: v[i])
        r = [0.0] * len(v)
        for rank, i in enumerate(order):
            r[i] = float(rank)
        return r
    ra, rb = ranks(a), ranks(b)
    n = len(a)
    ma, mb = sum(ra) / n, sum(rb) / n
    cov = sum((x - ma) * (y - mb) for x, y in zip(ra, rb))
    va = math.sqrt(sum((x - ma) ** 2 for x in ra))
    vb = math.sqrt(sum((y - mb) ** 2 for y in rb))
    return cov / (va * vb) if va and vb else float("nan")


def load_rows(stats_csv):
    """-> {(trace, exp): {ipc, precision, recall}} for Filter==1 rows."""
    rows = {}
    with open(stats_csv) as f:
        for r in csv.DictReader(f):
            if r.get("Filter", "").strip() != "1":
                continue
            try:
                ipc = float(r["ipc"])
            except (KeyError, ValueError):
                continue
            if ipc <= 0:
                continue
            def opt(k):
                try:
                    return float(r[k])
                except (KeyError, ValueError):
                    return None
            rows[(r["TraceName"], r["ExpName"])] = {
                "ipc": ipc, "precision": opt("precision"), "recall": opt("recall"),
            }
    return rows


def score_window(manifest, rows, window, baselines):
    """Score all role=config exps of one window. baselines: {trace:{nopf,pythia}}."""
    exps = [e for e, m in manifest.items()
            if m["window"] == window and m["role"] == "config"]
    all_traces = sorted({t for (t, _) in rows})
    common = [t for t in all_traces
              if t in baselines
              and all((t, e) in rows for e in exps)]
    dropped = [t for t in all_traces if t not in common]
    scored = []
    for e in exps:
        ratios_n = [rows[(t, e)]["ipc"] / baselines[t]["nopf"] for t in common]
        ratios_p = [rows[(t, e)]["ipc"] / baselines[t]["pythia"] for t in common]
        prec = [rows[(t, e)]["precision"] for t in common
                if rows[(t, e)]["precision"] is not None]
        rec = [rows[(t, e)]["recall"] for t in common
               if rows[(t, e)]["recall"] is not None]
        worst_i = min(range(len(common)), key=lambda i: ratios_p[i])
        scored.append({
            "exp": e,
            "key": manifest[e]["key"],
            "n_traces": len(common),
            "geo_nopf": geomean(ratios_n),
            "geo_pythia": geomean(ratios_p),
            "worst_vs_pythia": ratios_p[worst_i],
            "worst_trace": common[worst_i],
            "mean_prec": sum(prec) / len(prec) if prec else None,
            "mean_recall": sum(rec) / len(rec) if rec else None,
            "flag": ratios_p[worst_i] < 0.98,
        })
    scored.sort(key=lambda s: -s["geo_nopf"])
    return scored, common, dropped


def main():
    cdir = os.path.abspath(sys.argv[1])
    try:
        rnd = int(sys.argv[2])
        rname = f"round{rnd:02d}"
    except ValueError:
        rnd = sys.argv[2]          # sweep rounds are named, e.g. "sweep01"
        rname = sys.argv[2]
    stats_csv = sys.argv[3]
    rdir = os.path.join(cdir, "rounds", rname)
    manifest = json.load(open(os.path.join(rdir, "manifest.json")))
    rows = load_rows(stats_csv)

    # baselines: from this round's rows (round 1) or the campaign file
    def extract_baselines(window):
        b = {}
        nm = {m["role"]: e for e, m in manifest.items() if m["window"] == window
              and m["role"].startswith("baseline")}
        if len(nm) < 2:
            return None
        for (t, e), v in rows.items():
            if e == nm["baseline_nopf"]:
                b.setdefault(t, {})["nopf"] = v["ipc"]
            elif e == nm["baseline_pythia"]:
                b.setdefault(t, {})["pythia"] = v["ipc"]
        return {t: d for t, d in b.items() if "nopf" in d and "pythia" in d}

    bl_path = os.path.join(cdir, "baselines.json")
    baselines = extract_baselines("tuning")
    if baselines:
        if not os.path.exists(bl_path):
            with open(bl_path, "w") as f:
                json.dump(baselines, f, indent=1)
    else:
        baselines = json.load(open(bl_path))

    scored, common, dropped = score_window(manifest, rows, "tuning", baselines)

    with open(os.path.join(rdir, "rollup.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(scored[0].keys()))
        w.writeheader()
        w.writerows(scored)

    top5, seen = [], set()
    for s in scored:
        if s["key"] not in seen:
            seen.add(s["key"])
            top5.append(s)
        if len(top5) == 5:
            break
    with open(os.path.join(rdir, "top5.json"), "w") as f:
        json.dump(top5, f, indent=1)

    # ---- report ------------------------------------------------------------
    L = [f"# Campaign1 round {rnd} rollup", ""]
    L += [f"- traces scored: {len(common)} / dropped: {len(dropped)}"
          + (f" ({', '.join(dropped)})" if dropped else "")]
    L += [f"- configs scored: {len(scored)}", ""]

    fid_bl = extract_baselines("full")
    if fid_bl:  # round-1 window-fidelity check
        full_scored, fc, fd = score_window(manifest, rows, "full", fid_bl)
        by_key_t = {s["key"]: s["geo_nopf"] for s in scored}
        by_key_f = {s["key"]: s["geo_nopf"] for s in full_scored}
        keys = [k for k in by_key_t if k in by_key_f]
        rho = spearman([by_key_t[k] for k in keys], [by_key_f[k] for k in keys])
        top10_t = {s["key"] for s in scored[:10]}
        top10_f = {s["key"] for s in full_scored[:10]}
        L += ["## Window fidelity (50M+200M vs 100M+500M)",
              f"- Spearman rank correlation over {len(keys)} configs: **{rho:.3f}**",
              f"- top-10 overlap: {len(top10_t & top10_f)}/10", ""]

    L += ["## Ranking (tuning window, geomean IPC speedup)", "",
          "| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |",
          "|---|-----|---------|-----------|-------------------------|------|--------|------|"]
    for i, s in enumerate(scored[:20]):
        pr = f"{s['mean_prec']:.3f}" if s["mean_prec"] is not None else "-"
        rc = f"{s['mean_recall']:.3f}" if s["mean_recall"] is not None else "-"
        L += [f"| {i+1} | {s['key']} | {s['geo_nopf']:.4f} | {s['geo_pythia']:.4f} "
              f"| {s['worst_vs_pythia']:.3f} ({s['worst_trace']}) | {pr} | {rc} "
              f"| {'FLAG' if s['flag'] else ''} |"]
    nflag = sum(1 for s in scored if s["flag"])
    L += ["", f"- flagged configs (worst-trace < 0.98 vs pythia): {nflag}",
          "", "## Default top-5 (pending judgment in decision.md)", ""]
    L += [f"{i+1}. `{s['key']}`  geo_nopf={s['geo_nopf']:.4f}"
          for i, s in enumerate(top5)]
    with open(os.path.join(rdir, "report.md"), "w") as f:
        f.write("\n".join(L) + "\n")
    print(f"scored={len(scored)} common_traces={len(common)} "
          f"dropped={len(dropped)} report={os.path.join(rdir, 'report.md')}")


if __name__ == "__main__":
    main()
