# sweep04 decision — weight-table sizes on f20+f21+f22 @ act +2 — 2026-07-07 ~07:20 UTC

## VERDICT: owner's hypothesis confirmed — f21 is size-INSENSITIVE.

Baseline (64k-uniform, act +2): 84.0% prec / 76.5% recall / 1.0223 geo.

| sizes (f20 x f21 x f22) | prec% | recall% | geo | Δprec |
|--------------------------|-------|---------|-----|-------|
| 64k x **16k** x 64k | 83.9 | 76.3 | 1.0220 | −0.1 |
| 64k x **4k** x 64k | 83.9 | 76.2 | 1.0207 | −0.1 |
| 64k x **1k** x 64k | 83.8 | 75.5 | 1.0198 | −0.2 |
| **16k-uniform** | 83.8 | 76.2 | 1.0209 | −0.2 |
| 4k-uniform | 83.3 | 75.6 | 1.0200 | −0.7 |
| 1k-uniform | 82.5 | 74.8 | 1.0207 | −1.5 |
| f20→1k (others 64k) | 83.4 | 75.9 | 1.0215 | −0.6 |
| f22→1k (others 64k) | 83.3 | 76.0 | 1.0216 | −0.7 |

- **f21 (PageMissRatio)**: 1,024 entries suffice (−0.2pp) — consistent with
  its 2^10 natural index space. Hypothesis confirmed.
- **f20 (footprint hash)** and **f22 (delta signature)**: both want ≥16k
  (1k costs −0.6/−0.7pp); their hashed domains genuinely use the space.
- **Smallest measured config within 0.2pp of the 64k baseline: 16k-uniform**
  (48k total entries, 4x smaller). The synthesized hardware pick
  16k/1k/16k (~33k entries, ~6x smaller than 192k) is bounded by the
  one-at-a-time results at ≈−0.2..−0.3pp — worth one confirmation run if the
  owner wants that exact point.
- Suite geo stays within 1.0198–1.0223 across ALL size configs — table size
  is a precision knob, not an IPC knob, at these scales.

Data: 600/600 rows valid, 50/50 traces.
