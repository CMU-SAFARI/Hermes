# val03 decision — FULL-WINDOW finalist confirmation — 2026-07-07 ~18:40 UTC

## VERDICT: PASS — the window-fidelity caution is RETIRED.

Finalists at 100M+500M on the full trace population (146/147; see note):

| config | tuning window (147t) | FULL window (146t) | Δprec | Δgeo |
|--------|----------------------|--------------------|-------|------|
| f20+f21+f22 @ +2 | 83.2/76.2/1.0164 | 82.8/77.3/**1.0190** | −0.3 | +0.0026 |
| f20+f21+f22 @ +6 | 84.5/72.8/1.0154 | 84.5/73.9/1.0183 | ±0.0 | +0.0029 |
| f20+f21+f22 @ +8 | 85.2/71.2/1.0157 | **85.2**/71.9/1.0177 | −0.1 | +0.0020 |
| f21+f22 @ +7 (ref) | 83.4/71.2/1.0147 | 83.4/71.9/1.0175 | ±0.0 | +0.0028 |

- Precision agrees to ±0.3pp; recall +0.7–1.1pp; **geo +0.2–0.3pp HIGHER at
  the full window** — the 50M+200M tuning window slightly UNDER-stated the
  gains; every decision made on it is conservative-safe.
- Rankings unchanged: the triple remains geo-best at +2 and dominates the
  pair at matched precision (at ~72% recall: 85.2% vs 83.4% precision at
  equal geo).
- The r01 fidelity tripwire (aggregate Spearman 0.784) is hereby closed with
  direct evidence at the exact configs that matter: the operator's
  tie-shuffling diagnosis was correct.

Note: trace 853.ns3_s-tcp_validation-202B dropped (3/6 exps walltime-killed
at 12h; single-trace common-drop). An optional 14h retry can restore 147/147
for the thesis tables — not blocking any decision.

## Campaign status: ALL RESULTS IN. Owner decisions pending:
1. Operating point on the triple's curve (menu A–F + this full-window table;
   evidence: act +2 for IPC-lean, act +2 with pos_train=8/−21 for 86.5%
   precision at minimal cost, act +8 for the pure-act 85% floor).
2. Optional: exact-size confirmation run (16k/1k/16k per sweep04).
3. Campaign close-out writeup.
