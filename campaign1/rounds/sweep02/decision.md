# sweep02 decision — 2026-07-07 ~01:40 UTC (autonomous)

## The complete f20+f21+f22 activation curve (merged sweep01+sweep02, 50 traces)

| act | prec% | recall% | geo vs pythia |
|-----|-------|---------|---------------|
| -10 | 78.8 | 83.0 | 1.0218 |
| -4  | 81.6 | 79.7 | 1.0204 |
| -2  | 82.4 | 78.6 | 1.0196 |
|  0  | 83.3 | 77.6 | 1.0207 |
| +2  | 84.0 | 76.5 | **1.0223** (campaign-best geo) |
| +4  | 84.5 | 74.9 | 1.0202 |
| +6  | **85.1** | 73.4 | 1.0196 |
| +8  | 85.8 | 72.0 | 1.0213 |
| +10 | **86.4** | 69.7 | 1.0186 |

Precision crossings: **85% at act ≈ +6** (recall 73.4), **86% at act ≈ +10**
(recall 69.7). The curve is smooth and monotone in precision/recall; geo has
a broad plateau 1.019–1.022 — the suite-level cost of precision remains tiny.

## Domination check vs the extended pairs (from sweep02)

| config | prec% | recall% | geo |
|--------|-------|---------|-----|
| f20+f21+f22 @ +8 | 85.8 | **72.0** | **1.0213** |
| f20+f21 @ +11 | 85.5 | 69.8 | 1.0189 |
| f21+f22 @ +11 | 85.4 | 67.9 | 1.0186 |

The triple dominates both pairs at matched ~85.5% precision: more recall AND
more IPC. The sweep01 verdict holds across the entire measured range.

## Overnight launches (per owner authorization)

Best-2 activation points for downstream sweeps: **+2** (geo peak) and **+8**
(precision-side peak). Launched:
- **val02**: sweep02's 9 configs x 147 traces (cross-scale check of this curve).
- **sweep03**: pos/neg train-threshold grid on the triple at act {+2, +8} —
  pos {8,16,24,32} x neg {-7,-14,-21,-28} minus the rule point (30 configs).
- **sweep04**: weight-table sizes on the triple at act +2 — uniform
  {1k,4k,16k} + per-feature one-at-a-time reductions from the 64k baseline
  (12 configs). Tests the owner's f21-insensitivity hypothesis.
- **val03**: full-window (100M+500M) confirmation on 147 traces — triple at
  {+2, +6, +8} + f21+f22@+7 reference + both baselines (12h walltime).
  Retires the window-fidelity caution for the finalists.
