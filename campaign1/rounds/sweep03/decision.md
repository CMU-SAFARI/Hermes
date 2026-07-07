# sweep03 decision — train-threshold grid on f20+f21+f22 — 2026-07-07 ~08:10 UTC

## VERDICT: pos_train is a superior precision lever to the activation threshold.

No grid point beats the rule (24/−21) on BOTH precision and recall at the
same activation — the grid trades recall for precision, as expected. But on
the COMBINED frontier, the train lever dominates the act lever wherever they
compete:

| target precision | via act only (rule train) | via train lever | advantage |
|------------------|----------------------------|-----------------|-----------|
| ~85.6% | act +8: 72.0 recall, 1.0213 | act +2, tr 8/−14: **74.5 recall**, 1.0206 | +2.5pp recall |
| ~86.5% | act +10: 69.7 recall, 1.0186 | act +2, tr 8/−21: **72.2 recall**, **1.0210** | +2.5pp recall, +0.24% geo |
| ~87–90% | (act range exhausted) | act +2, tr 8/−28: 87.1/68.8/1.0192 · act +8, tr 8/−7: 88.5/65.9/1.0191 · act +8, tr 8/−28: 89.8/58.3/1.0167 | frontier extended |

Mechanism: pos_train=8 stops reinforcing weights early (sum saturates low),
so only genuinely confident predictions clear the threshold — sharper
separation than shifting the decision boundary. neg_train matters less
(second-order at fixed pos).

## The operating-point menu for the owner (f20+f21+f22, 50-trace numbers)

| # | config | prec% | recall% | geo vs Pythia |
|---|--------|-------|---------|---------------|
| A | act +2, rule 24/−21 | 84.0 | 76.5 | **1.0223** (geo peak) |
| B | act +2, tr 8/−14 | 85.6 | 74.5 | 1.0206 |
| C | act +2, tr 8/−21 | 86.5 | 72.2 | 1.0210 |
| D | act +2, tr 8/−28 | 87.1 | 68.8 | 1.0192 |
| E | act +8, tr 8/−7 | 88.5 | 65.9 | 1.0191 |
| F | act +8, tr 8/−28 | 89.8 | 58.3 | 1.0167 |

Recommendation shape: B or C — >85% precision (fotonik-class traces near
break-even per the sweep01 canary trend) while keeping recall in the
low-to-mid 70s and geo within 0.15% of the peak.

Data: 1,500/1,500 after retry merge (48 node-loss rows recovered), 50/50
traces. Note for ops: two whole-trace ID-block losses tonight (sqlite here,
llvm in val02) — node failures, not timeouts; retries recovered all.
