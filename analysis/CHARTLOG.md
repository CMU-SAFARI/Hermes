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
