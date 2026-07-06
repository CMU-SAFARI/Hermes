# sweep01 decision — 2026-07-06 ~22:40 UTC (autonomous)

## VERDICT: the probe triple `f20+f21+f22` DOMINATES `f21+f22` in the
## precision region — the third feature earns its place.

Phase-2 decision rule ("does any probe triple's curve dominate f21+f22 at
precision >= 82%?") fires YES:

| operating point | prec% | recall% | geo vs pythia | fotonik vsP (prec) | graph3313 vsP (prec) |
|-----------------|-------|---------|---------------|--------------------|----------------------|
| f21+f22 @ act +7 | 84.0 | 72.0 | 1.0193 | 0.980 (86.0) | 0.979 (81.6) |
| **f20+f21+f22 @ act +2** | **84.0** | **76.5** | **1.0223** | **0.980 (89.0)** | 0.974 (80.7) |

At matched 84% precision the triple keeps **+4.5pp recall**, gains **+0.3%
suite geomean** (1.0223 is the best geo measured in the whole campaign,
beating the round-2 winner's 1.0218), and posts the best fotonik3d local
precision seen anywhere (89.0%). The domination holds across the >=82%
region (interpolating the triple's -4..+2 segment vs the pair's +1..+7).

Runner-up notes:
- `f20+f21` (pair) is the canary champion (fotonik 0.982 / 90.0% local
  precision at act +7) but pays ~0.5pp suite precision and its geo curve
  tops at 1.0202 — dominated by the triple on suite numbers.
- `f21+f22+f27@17` and `f18+f21+f22` mildly beat the pair at +2 but do not
  match the f20 triple. `f21+f22+f30@20` adds nothing.

## The f21+f22 curve itself (for the record)

Threshold does exactly what the bandwidth analysis predicted: act -15 -> +7
trades recall 90.1 -> 72.0 for precision 63.2 -> 84.0; fotonik3d recovers
0.930 -> 0.980 and graph500-3313B 0.940 -> 0.979, while the suite geomean
moves only 1.0174 -> 1.0193 (peak 1.0219 at act -5). Precision is a nearly
free knob at suite level and a decisive one on the bandwidth canaries.

## Actions taken (per the Phase-2 autonomous mandate)

1. This decision + curves published; owner reviews in the morning.
2. **sweep02 launched** (~450 jobs): densifying the interesting region —
   f20+f21+f22 at act {-2, 0, +4, +6, +8, +10} (6 new points around the
   dominant +2), f21+f22 at {+9, +11} and f20+f21 at {+11} (extending both
   comparison curves past 84% precision). Completes the operating-point menu
   for the owner's precision-floor decision.
3. val01 (147 traces, in flight) carries the same sweep01 configs — the
   cross-scale check of this verdict lands with it.

## Pending owner decisions (morning)

- Final config + operating point (current evidence points to f20+f21+f22 at
  act between +2 and +6, pending sweep02's denser curve and val01's
  147-trace confirmation).
- Precision floor for the "justifiable recall trade-off".
- Then: train-threshold sweep and table-size sweep on the chosen config
  (queued), and the finalist full-window confirmation run.
