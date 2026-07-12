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
| 2026-07-12 | charts/accuracy_coverage_membound146_bw3200.{png,pdf} | scripts/plot_accuracy_coverage_membound146.R | membound146_bw3200 (raw prec/recall) -> accuracy_coverage_bw3200 (per-cat AVG) | 2 facets Accuracy(=precision)/Coverage(=recall); XPT + Hermes-UnC standalone; non-weighted arith mean; AVG labelled; XPT accuracy on n=145 (1 zero-prediction trace excluded from that mean only) |
