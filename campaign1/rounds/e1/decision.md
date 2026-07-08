# E1 decision — train grid at FULL window x 145 traces — 2026-07-08 ~07:15 UTC

## VERDICT: the operating point act+2, tr 8/-21 is CONFIRMED at the full window.

| config (act+2) | prec% | recall% | geo vs pythia |
|----------------|-------|---------|----------------|
| rule 24/-21 (val03 ref, 146t) | 82.8 | 77.3 | 1.0190 |
| tr 8/-14 | 85.2 | 74.3 | 1.0189 |
| **tr 8/-21 (shipping)** | **86.3** | 71.4 | 1.0189 |
| tr 8/-28 | 87.2 | 67.6 | 1.0176 |
| act+8 tr 8/-21 (max precision) | 89.7 | 59.7 | 1.0171 |

- The 50-trace sweep03 numbers transferred to FW x145 within 0.2-0.8pp:
  the shipping point lands 86.3p/71.4r at geo parity with the rule
  (1.0189 vs 1.0190) — +3.5pp precision effectively free in IPC.
- FW precision crossings, all at act+2: 85% @ tr8/-14, 86% @ tr8/-21,
  87% @ tr8/-28; beyond 88% requires act+8 (recall <66%).
- Note: trace 765.roms_r-benchmark2-985B common-dropped (its last 3 jobs
  still running at scoring time; 145/146). Re-merge to 146 in CLOSEOUT if
  they land before E3 does — verdict is insensitive either way.

Data: 2,625/2,628 valid at scoring (3 pending, single trace).
