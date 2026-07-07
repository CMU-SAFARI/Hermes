# val01 decision — cross-scale validation (50 → 147 traces) — 2026-07-07 ~03:20 UTC

## VERDICT: the 50-trace suite's conclusions hold on the full 147-trace population.

- **Winner unchanged**: `f20+f21+f22 @ act +2` is the best config on 147
  traces by BOTH geo vs Pythia (1.0164, highest of all 30) and precision
  (83.2%, highest of all 30).
- **Domination preserved at matched precision**: triple@+2
  (83.2p / 76.2r / 1.0164) vs f21+f22@+7 (83.4p / 71.2r / 1.0147) —
  ~+5pp recall and +0.17% geo at the same precision. Same picture vs f20+f21.
- **Curve shapes parallel**: activation ordering identical at both scales;
  precision shifts a uniform −0.6..−1.6pp and recall ±1pp.
- **Absolute geo drops ~0.5pp uniformly** (e.g., winner 1.0223 → 1.0164):
  the MPKI-selected 50-trace suite slightly overstates absolute Hermes gains,
  as expected. Rankings — the thing the tuning used — are scale-stable. For
  the thesis, quote the 147-trace numbers as headline results.

Full 30-row 50-vs-147 table in report.md / rollup.csv beside this file.

Data quality: 4,704/4,704 rows valid (zero TIMEOUTs; the 6h walltime held on
the full tlist), 147/147 traces scored, zero drops.
