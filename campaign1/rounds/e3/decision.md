# E3 decision — unconstrained-storage reference — 2026-07-08 ~11:40 UTC

## Valid results (tuning window, 146 traces, act+2 tr 8/-21):

| config | prec% | recall% | geo vs nopf | geo vs pythia |
|--------|-------|---------|-------------|----------------|
| 6K-sizes (4096/1024/1024) | 85.1 | 69.0 | 1.1611 | 1.0150 |
| 64k-uniform | 86.0 | 71.3 | 1.1624 | 1.0160 |

The 32x table compression costs 0.9pp precision / 2.3pp recall / 0.10% geo at
146-trace scale — consistent with E2's 50-trace estimate (-0.8pp).

## OPERATOR ERROR disclosed: the _fw twin runs in this batch were invalid.

The full-window twins were built by sed-ing exp LINES, but the window lives
in the $(TBASE) macro DEFINITION — the substitution never matched, the twins
silently re-ran the tuning window (identical prec/recall proves it), and
292 jobs were duplicates (~0.24% of budget). val03's full-window runs are
NOT affected (different, correct construction). Fix: _fw exps now reference
a $(WBASE) definition; verification step added (assert _fw lines reference
WBASE before submit). The full-window reference rows for both configs above
are FOLDED INTO E3' instead of a separate rerun.
