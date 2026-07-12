# Chart log — Hermes uncore results (started 2026-07-12)

Data ledger: hermes_results.xlsx (INDEX sheet maps sheets -> sources ->
charts). One entry below per generated chart.

| date | chart file | script | workbook sheet(s) | notes |
|------|-----------|--------|-------------------|-------|
| 2026-07-12 | charts/speedup_membound146_bw3200.{png,pdf} | scripts/plot_speedup_membound146.R (adapted from runs/cluster/plot_speedup.R) | membound146_bw3200 | 5 configs vs nopref; categories specrate-fp/int, specspeed-fp/int, GEOMEAN (unweighted geomean per category); geo-SD error bars; numbers in charts/speedup_membound146_bw3200_numbers.csv |

## Style conventions (established 2026-07-12) — apply to ALL paper charts
- Canonical palette + config order live in scripts/hermes_style.R; every
  chart script `source()`s it so colors are identical across figures.
  Palette: grey=Pythia, orange family=XPT (light=alone, strong=+Pythia),
  blue family=Hermes-UnC (light=alone, strong=+Pythia); greens reserved
  for Hermes size variants. Edit hermes_style.R once to restyle everything.
- NO error bars unless explicitly requested.
- "Hermes-UnC" = Normal variant by default.

Update: speedup_membound146_bw3200 chart re-generated 2026-07-12 — dropped
error bars, switched to hermes_style.R palette.

Update: speedup_membound146_bw3200 re-gen 2026-07-12 — canonical bar order
changed to XPT, Hermes-UnC, Pythia, XPT+Pythia, Hermes-UnC+Pythia (in
hermes_style.R, propagates to all charts); GEOMEAN bars value-labelled
(1.014/1.058/1.151/1.149/1.171).
| 2026-07-12 | charts/accuracy_coverage_standalone_membound146_bw3200.{png,pdf} | scripts/plot_accuracy_coverage_membound146.R | membound146_bw3200 -> acc_cov_standalone_bw3200 | XPT & Hermes-UnC standalone; Accuracy/Coverage facets; arith mean; benefit-of-doubt: no-prediction=100% accuracy |
| 2026-07-12 | charts/accuracy_coverage_pythia_membound146_bw3200.{png,pdf} | scripts/plot_accuracy_coverage_membound146.R | membound146_bw3200 -> acc_cov_pythia_bw3200 | XPT+Pythia & Hermes-UnC+Pythia (predictors atop Pythia); same rule; XPT AVG 65.4 acc / 5.4 cov vs Hermes 85.4 / 69.8 || 2026-07-12 | charts/dram_increase_membound146_bw3200.{png,pdf} | scripts/plot_dram_increase_membound146.R | dram_reqs_bw3200 (raw) -> dram_pct_increase_bw3200 | %inc DRAM read reqs (RQ row-buffer hit+miss, writes excluded) over nopref; 5 configs; arith mean per cat; AVG labelled (XPT 0.1 / Hermes 3.7 / Pythia 29.0 / XPT+P 29.6 / Hermes+P 35.2). Raw re-extracted from bwsweep .out (not in IPC rollup) |
| 2026-07-12 | charts/bwsweep_curve_membound146.{png,pdf} | scripts/plot_bwsweep_curve_membound146.R | bwsweep_ipc_all (raw) -> bwsweep_speedup_curve | LINE chart: geomean speedup vs nopref (same-MTPS baseline) across 200..6400 MT/s (log2 x); 5 lines Pythia/XPT/XPT+P/Hermes-UnC/Hermes-UnC+P; NO new runs (bwsweep data). Crossover 400-800 MT/s; n=141/144/146x4 |
