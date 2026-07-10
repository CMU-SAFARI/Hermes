# Hermes@Uncore — Final Single-Core Projection (Architect Handover)

2026-07-10. Full SPEC26 rate-int (144 traces, 13 workloads) + speed-int
(260 traces, 11 workloads); 100M warmup + 500M sim; frozen configs
`config/hermes_uncore_{lite,normal,big}.ini` (rbdev fc49264); XPT =
Intel-style physical-page tracker (`config/ocp_xpt.ini`, 256x256,
threshold 32) at the same uncore placement with physical address. DDRP
action enabled for every predictor. 4,030/4,040 runs valid — one trace
(821.gcc_s-lz4hc_0018-0B) hits a pre-existing trace-specific simulator
deadlock on the frozen baseline binary in ALL experiments (incl. nopf) and
is common-dropped; its workload's other simpoints carry its weight.

Rubric (owner-specified): per-workload = SimPoint-WEIGHTED GEOMETRIC mean
of per-trace speedups / WEIGHTED ARITHMETIC mean of precision/recall;
suite = unweighted geomean (speedups) / mean (P,R) across workloads.

## COMBINED (24 workloads)

| config | speedup vs nopf | vs Pythia | precision% | recall% |
|--------|-----------------|-----------|------------|---------|
| Pythia alone | 1.0295 | 1.0000 | — | — |
| **Hermes-Lite** | 1.0158 | 0.9867 | 94.0 | 87.0 |
| **Hermes-Normal** | 1.0155 | 0.9864 | 93.7 | 87.5 |
| **Hermes-Big** | 1.0154 | 0.9863 | 93.6 | 87.7 |
| **Hermes-Lite + Pythia** | 1.0340 | **1.0044** | 82.3 | 65.2 |
| **Hermes-Normal + Pythia** | 1.0345 | **1.0048** | 82.8 | 66.3 |
| **Hermes-Big + Pythia** | **1.0348** | **1.0052** | 83.2 | 66.5 |
| XPT | 1.0039 | 0.9752 | 92.3 | 20.2 |
| XPT + Pythia | 1.0296 | 1.0001 | 59.1 | 7.5 |

## Per-suite headline (speedup vs nopf | vs Pythia)

| config | rate-int (13 wl) | speed-int (11 wl) |
|--------|------------------|-------------------|
| Pythia | 1.0360 \| 1.0000 | 1.0218 \| 1.0000 |
| Hermes-Big | 1.0173 \| 0.9819 | 1.0131 \| 0.9916 |
| Hermes-Big + Pythia | 1.0423 \| 1.0060 | 1.0261 \| 1.0043 |
| XPT | 1.0041 \| 0.9692 | 1.0036 \| 0.9823 |
| XPT + Pythia | 1.0362 \| 1.0001 | 1.0219 \| 1.0002 |

(Full 10-row per-suite and per-workload tables: rate_/speed_/combined_
{suite,workloads}.csv in this directory.)

## The three headline contrasts

1. **Hermes vs XPT, standalone.** At equal placement and near-equal
   precision (93.6 vs 92.3), Hermes covers **87.7%** of off-chip loads to
   XPT's **20.2%** — 4.3x the coverage — which converts to ~4x the
   performance (+1.54% vs +0.39% over no-prefetch).
2. **Hermes vs XPT, on top of Pythia — the decisive one.** With Pythia
   removing the easy misses, XPT's page-counter starves: recall collapses
   to 7.5%, precision to 59%, and its performance contribution to
   **+0.01%**. Hermes's program-behavior features stay informative:
   66% recall at 83% precision, still worth **+0.44..+0.52%** on top of a
   state-of-the-art prefetcher. Best overall config: Hermes-Big+Pythia at
   **+3.48%** over no-prefetch.
3. **Version ladder at single core.** Lite/Normal/Big are within 0.05%
   of each other on the full suite — storage buys prediction quality
   (recall 87.0 -> 87.7 standalone; 65.2 -> 66.5 with Pythia) but not
   single-core suite-level speed. Choose the version by storage budget
   and multi-core behavior (campaign-2), not single-core performance;
   Lite is the single-core sweet spot.

## Context for reading the numbers

- Campaign-1's headline (+1.8% over Pythia) was measured on its
  146-trace tuning list — a memory-intensive selection scored as a
  per-trace geomean. This handover covers the FULL suites under
  SimPoint-weighted per-workload scoring, which dilutes gains with
  workloads that rarely leave the chip. Same predictor, honest
  full-suite accounting — both numbers are correct answers to different
  questions.
- Pythia alone (+2.95%) exceeds Hermes alone (+1.6%): expected — a
  prefetcher removes stalls outright; an off-chip predictor only
  accelerates the DRAM fetch of loads that still miss. Their composition
  (+3.48%) is the point.
- P/R with Pythia is measured against the post-prefetch miss stream
  (harder, rarer events) — the drop vs standalone is a property of the
  task, not degradation.

## Provenance

Batches 20260709T075351Z_projection_rate + 20260709T075505Z_projection_speed
(repo Hermes @ rbdev fc49264, byte-identical semantics to the b18be0c
campaign-1 pin). Scorer: projection/scripts/collect_proj.py (self-tested;
nan-precision phases = zero prediction opportunity, excluded from P/R
means). All artifacts published on origin/tuning under projection/.
