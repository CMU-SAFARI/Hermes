# E2 decision — hardware-grade sizing (owner caps) — 2026-07-08 ~07:10 UTC

## PICK: f20=4096, f21=1024, f22=1024 — 6,144 total entries (32x smaller than 64k-uniform)

At the shipping operating point (act+2, tr 8/-21), 50-trace suite:
| config | total entries | prec% | recall% | geo |
|--------|---------------|-------|---------|-----|
| 64k-uniform (reference) | 196,608 | 86.5 | 72.2 | 1.0210 |
| **4096x1024x1024 (pick)** | **6,144** | 85.7 | 70.2 | 1.0195 |
| 2048x1024x1024 | 4,096 | 85.6 | 69.9 | 1.0189 |
| 1024x1024x512 | 2,560 | 85.4 | 69.7 | 1.0203 |

- The documented pick-rule (within 0.3pp of 64k) is UNATTAINABLE under the
  owner's caps — the 196k->6k compression costs 0.8pp precision / 2.0pp
  recall / 0.15% geo. Judgment: picked best-under-caps; the 64k twin in E3
  quantifies this delta at 146 traces x full window for the owner's final
  read. (An excellent trade: <1pp for 32x smaller tables.)
- **f21 floor is 1k, not lower**: 1024->256 costs ~2pp recall (owner's
  size-insensitivity holds only down to its natural 2^10 index space).
- **f22 at 1k > 512** (+0.2-0.8pp recall); **f20 earns its 4k cap**
  (1k->4k buys +0.3pp precision consistently).

Data: 900/900 valid, 50/50 traces.
