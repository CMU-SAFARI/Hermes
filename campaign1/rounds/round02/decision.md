# Round 2 decision — 2026-07-06 ~12:00 UTC (autonomous)

## CONVERGENCE DETERMINATION

Best round-2 config: `f21+f22` (PageMissRatio + LastNDeltas), geo vs nopf
**1.163123** (1.0218 vs pythia-alone).
Best round-1 config: `f21`, geo **1.161138**.
Relative improvement: **+0.1710% < epsilon 0.2%** (protocol stop rule, set by
owner 2026-07-03) -> **the campaign converges at round 2.** No round 3.

Not a degenerate stop: 128/265 extensions beat their beam parent — pairs
genuinely help, just below the threshold that the owner defined as "worth
another 13k-job round".

## Winner and the Pareto reading

- Protocol winner (argmax): `f21+f22` = PageMissRatio + LastNDeltas at
  1.163123 (+2.18% over pythia-alone geomean).
- Pareto note for hardware costing: `f21` ALONE achieves 1.161138 — 98.3% of
  the pair's total Hermes gain with a single 2^10-state feature. The pair
  adds +0.17% for a second table + delta-history register. Owner picks the
  operating point; both are fully characterized in the memo.

## Observations (for the thesis)

- The winning pair combines page *outcome history* (miss-ratio pair) with
  page *access pattern* (4-delta stride signature) — complementary signals.
- Predictor quality improved markedly with pairs (recall 75.8 -> 81.3 at
  ~equal precision ~79) while IPC moved only +0.17%: at the uncore, under
  Pythia, with DDRP as the action, PREDICTION IS NO LONGER THE BOTTLENECK —
  the action's latency-saving headroom is. Feature engineering beyond two
  features pays off in accuracy, not in end-to-end IPC.
- Sibling concentration in the top-5 (f20 parent in 4/5) would have
  triggered the 3-children/parent cap had we continued — moot now.
- Universal fotonik3d/graph500 worst-trace flags persist across ALL 265
  configs (DDRP bandwidth cost, feature-independent), unchanged from r01.

## Operational record

- 64/13,250 rows were genuine 4h TIMEOUTs (pair-configs run ~15% slower than
  singletons; killed at ~88% of 250M). All retried at 6h in batch
  20260706T063412Z_campaign1_r02_retry: 270/270 valid. Final scoring set:
  50/50 traces x 265 configs, zero drops.
- Walltime lesson recorded: walltime must scale with feature count.
- Budget: 19,476 / 120,000 jobs used. Convergence at round 2 leaves ~100k
  budget unspent.

## Next

FINAL_REPORT.md follows once the r01 window-fidelity twins drain (they are
in their last running wave) — the fidelity verdict belongs in the final
report since it qualifies every short-window number in this campaign.
