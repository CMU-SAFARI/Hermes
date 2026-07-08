# E4 decision — joint PB x weight storage sweep — 2026-07-08 ~18:05 UTC

## THE PICKS (best precision-first under each byte line; PB entry ~14B, weight 5b)

| version | PB (sets x assoc) | weights (f20/f21/f22) | total | prec% | recall% | geo vs Pythia |
|---------|-------------------|------------------------|-------|-------|---------|----------------|
| **Hermes-Lite** | 8x16 (128e) | 2048/1024/1024 (4K) | **4.25 KB** | 84.8 | 68.3 | 1.0180 |
| **Hermes-Normal** | 32x16 (512e) | 8192/1024/4096 (13.3K) | **15.1 KB** | 85.6 | 70.6 | 1.0185 |
| **Hermes-Big** | 64x16 (1024e) | 8192/1024/16384 (25.6K) | **29.6 KB** | 86.3 | 70.8 | 1.0194 |

A clean monotone ladder (precision 84.8 -> 85.6 -> 86.3; recall 68.3 -> 70.6
-> 70.8) — and Big at 29.6KB comes within 0.2pp precision of the fully
unconstrained 148KB configuration. (50-trace numbers; E3' produces the
146-trace both-window headline.)

## Isolation-row verdict: the PB-dominance hypothesis is NOT confirmed —
## and that's good news.

At 64k weights, shrinking the PB 2048 -> 32 entries costs only 1.2pp
precision (86.5 -> 85.3) — the page buffer degrades remarkably gracefully.
Per BYTE, weight tables are the more efficient precision resource (~+1.2pp
per ~14KB of weights vs ~+1.2pp per ~28KB of PB), because a PB entry costs
~22x a weight entry. Recall leans slightly PB-ward (best iso recall 72.1 at
PB 512). Consequence: the version splits are weight-leaning, PB modest —
the opposite of the pre-sweep hunch, which is exactly why we measured.
(Likely mechanism: even a small PB retains the HOT pages that generate most
LLC traffic; the long tail of cold pages contributes few predictions.)

## Geometry probes: null result (useful)

At 256 entries: 16x16 vs 32x8 vs 64x4 differ by <=0.1pp / <=0.0006 geo —
the PB is geometry-insensitive at these sizes; hardware may choose the
cheapest indexing.

## Data quality
2,550/2,550 rows valid (zero failures — cleanest batch of the campaign),
50/50 traces. Full 51-point Pareto in rollup.csv / report.md.
