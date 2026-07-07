# val02 decision — dense triple curve at 147 traces — 2026-07-07 ~08:50 UTC

## VERDICT: the operating-point curve generalizes; crossings shift ~one act step.

| act | 50-trace prec/recall/geo | 147-trace prec/recall/geo |
|-----|--------------------------|----------------------------|
| -10 | 78.8/83.0/1.0218 | 77.3/83.6/1.0153 |
| -4  | 81.6/79.7/1.0204 | 80.5/80.1/1.0160 |
| -2  | 82.4/78.6/1.0196 | 81.4/78.7/1.0156 |
|  0  | 83.3/77.6/1.0207 | 82.3/77.6/1.0159 |
| +2  | 84.0/76.5/1.0223 | 83.2/76.2/**1.0164** (147t geo peak) |
| +4  | 84.5/74.9/1.0202 | 83.8/74.6/1.0155 |
| +6  | 85.1/73.4/1.0196 | 84.5/72.8/1.0154 |
| +8  | 85.8/72.0/1.0213 | **85.2**/71.2/1.0157 |
| +10 | 86.4/69.7/1.0186 | **86.0**/68.6/1.0148 |

- Curve shape parallel; precision shifts a uniform −0.6..−1.1pp on the full
  population; recall within ±1pp; geo compressed ~0.5pp (as in val01).
- **Crossings on 147 traces: 85% at act ≈ +8; 86% at act ≈ +10** (one step
  later than the 50-trace suite). If the owner sets the floor on 147-trace
  precision, pick the act point one step higher than the 50-trace menu
  suggests — or equivalently use the sweep03 train-lever (pos_train=8),
  which bought +2.5pp recall at matched precision on the 50-trace suite and
  should be confirmed at 147 traces for the final chosen point.
- act +2 remains the 147-trace geo peak (1.0164) — consistent winner.

Data: 1,323/1,323 after retry merge (48 node-loss rows recovered), 147/147
traces, zero drops. baselines.json verified restored to the 50-trace version.
