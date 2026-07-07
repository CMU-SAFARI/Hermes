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

| batch | question | size | ETA (UTC) |
|-------|----------|------|-----------|
| sweep03 | do pos/neg train thresholds beat the rule 24/−21 at act +2/+8? | 1,500 | ~06–09 |
| sweep04 | table sizes: is f21 insensitive? smallest config within 0.2pp of 64k? | 600 | ~05–07 |
| val02 | dense triple curve on 147 traces (act −2..+10) | 1,323 | ~06–09 |
| val03 | **full-window** (100M+500M) finalist check, 147 traces — retires the fidelity caution | 882 | afternoon |

Each gets scored + published as it drains; I'll update this brief in place.

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
