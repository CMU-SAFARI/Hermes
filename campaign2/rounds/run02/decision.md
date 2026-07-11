# Campaign-2 run02/run02b — Hermes-alone & XPT at 4c/1ch — 2026-07-11

Question (owner): with scarce per-core bandwidth, does high-precision
Hermes-ALONE beat speculative Pythia-ALONE? Plus XPT-alone / XPT+Pythia.
497/500 pairs valid; 3 heavy-mix stragglers backfilling (9-job retry in
flight; the three 99-mix rows below shift at most in the 4th decimal).

## Full 4-core / 1-channel matrix (100 mixes, vs same-mix baselines)

| config | vs nopf | vs Pythia | prec% | recall% |
|--------|---------|-----------|-------|---------|
| **Pythia alone** | **1.0689** | 1.0000 | — | — |
| XPT + Pythia | 1.0652 | 0.9972 | 64.5 | 7.0 |
| Hermes-Big + Pythia (embed) | 1.0633 | 0.9948 | 85.1 | 67.6 |
| Hermes-Big ALONE | 1.0238 | 0.9568 | **97.5** | **96.1** |
| Hermes-Normal ALONE | 1.0229 | 0.9568 | 97.4 | 96.0 |
| Hermes-Lite ALONE | 1.0221 | 0.9562 | 97.2 | 95.7 |
| XPT ALONE | 1.0042 | 0.9395 | 99.4 | 27.2 |

(+Pythia rows from run01; alone/XPT rows from run02/run02b.)

## VERDICT: the hypothesis is refuted — Pythia-alone beats Hermes-alone
## by ~4.4% even at scarce bandwidth. Prediction quality is not the
## bottleneck; value-per-correct-prediction is.

Hermes-alone is nearly a perfect predictor here (97.5p/96.1r — without a
prefetcher, the memory-intensive miss stream is easy). Yet near-perfect
prediction buys only +2.4%, a third of Pythia's +6.9%. Mechanism: a
correct off-chip prediction saves the ON-CHIP portion of the trip
(L2+LLC lookup, ~60-70 cycles) on a load that still pays full DRAM
latency — and under 1ch contention, DRAM queueing dominates the total,
shrinking the relative saving further. A correct prefetch removes the
ENTIRE stall. Bandwidth scarcity punishes Pythia's wasted prefetches,
but what remains of its benefit still dwarfs latency-shaving.

## Second observation: at 1ch, the WEAKER predictor composes better.

XPT+Pythia (1.0652) edges Hermes+Pythia (1.0633) — not because XPT
predicts better (7% recall!), but because its tiny coverage injects
almost no extra DDRP traffic. When bandwidth is the binding resource,
doing less costs less. This inverts at 2ch if bandwidth is the whole
story — the 2-channel batch (draining now) answers directly, and the
single-core bandwidth sweep (6 points, 200..6400 MTPS) will chart the
full crossover.

## Single-core anchor (projection, full suites, ample bandwidth):
Pythia 1.0295 > Hermes-alone ~1.016 > XPT-alone 1.004 — the same
ordering; contention compresses everything toward (and past) parity
with the baseline but does not reorder alone-configs.
