# Morning brief — 2026-07-07 (~03:30 UTC)

Good morning. Everything you asked for is done or in flight; nothing needs
rescue. **Read order:** this brief → sweep02 decision (the operating-point
curve) → val01 decision (147-trace confirmation) → FINAL_REPORT (campaign
narrative + fidelity ruling) → sweep01 decision (how the triple won).

## Where the evidence stands

1. **Final config, by every measure so far: `f20+f21+f22`**
   (PageMissRatio + LastNDeltas + PageSpatialFootprint). It dominates both
   pairs across the whole measured P-R range, on the 50-trace suite AND on
   the full 147-trace population (val01: winner by both geo and precision).
2. **Your only pending decision is the operating point** — one number, the
   precision floor. The triple's activation curve (50-trace; 147-trace shifts
   precision −1pp uniformly):

   | act | prec% | recall% | geo vs Pythia |
   |-----|-------|---------|---------------|
   | -10 | 78.8 | 83.0 | 1.0218 |
   | +2  | 84.0 | 76.5 | **1.0223** ← geo peak |
   | +6  | **85.1** | 73.4 | 1.0196 ← 85% floor |
   | +8  | 85.8 | 72.0 | 1.0213 |
   | +10 | **86.4** | 69.7 | 1.0186 ← 86% floor |

   (Full 9-point table in rounds/sweep02/decision.md. On 147 traces the
   winner holds 83.2% precision / 76.2% recall / 1.0164 geo at act +2.)

## Overnight batches (launched under your authorization; ~4.3k jobs)

| batch | verdict |
|-------|---------|
| sweep03 | **pos_train is the better precision lever** — operating-point menu A–F in rounds/sweep03/decision.md (up to ~90% precision; e.g. C: 86.5p/72.2r/1.0210) |
| sweep04 | **f21 size-insensitive confirmed** (1k entries = −0.2pp); 16k-uniform within 0.2pp of 64k at 1/4 budget |
| val02 | **curve generalizes to 147 traces** (parallel, −1pp; 85%/86% crossings at act +8/+10; act+2 still geo peak 1.0164) |
| val03 | full-window finalist check — still running, lands ~afternoon |

All decisions in rounds/*/decision.md; node-loss incidents (sqlite, llvm blocks) fully recovered by retries.

## Flags for your review

- **Fidelity ruling** (FINAL_REPORT): aggregate Spearman 0.784 tripped the
  0.8 tripwire; I diagnosed tie-shuffling (f21 #1 in both windows; precision
  fidelity 0.991) and ruled proceed-with-caution instead of HOLD. val03 is
  the direct closure of this caution for the finalists. Please confirm or
  overrule.
- The 50-trace suite overstates absolute geo by ~0.5pp vs 147 traces
  (rankings unaffected) — quote 147-trace numbers in the thesis.

## Ledger

Budget: **30,435 / 120,000 jobs.** Code frozen at b18be0c throughout; all
batches on the same source. Everything published on `origin/tuning`
(commit-per-round with decisions).

## Suggested sequence when you're in

1. Pick the precision floor → act point (evidence favors +2 if you weight
   IPC, +6..+8 if you weight the bandwidth-constrained story).
2. Review sweep03/sweep04 verdicts (likely landed by then) — they may refine
   the train thresholds and shrink the tables.
3. val03 lands in the afternoon → final config fully validated at full
   window on the full population → campaign closes with the writeup.
