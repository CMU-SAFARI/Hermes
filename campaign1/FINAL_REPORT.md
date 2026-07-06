# Campaign 1 — FINAL REPORT
### Beam search over PC-less off-chip-predictor features at the uncore
*(autonomous run 2026-07-03 → 2026-07-06; owner: Rahul Bera; operator: Claude)*

## Verdict

**Winner: `f21+f22` — PageMissRatio + LastNDeltas** at the uncore, physical
address, under Pythia with DDRP:

| config | geo IPC vs no-pref | geo IPC vs Pythia-alone | precision | recall |
|--------|--------------------|--------------------------|-----------|--------|
| `f21+f22` (winner) | **1.1631** | **1.0218** | 78.8% | 81.3% |
| `f21` alone (Pareto option) | 1.1611 | 1.0201 | 79.5% | 75.8% |
| Pythia-alone reference | 1.1381 | 1.0000 | — | — |

The beam converged in two rounds: 56 singletons (best `f21` 1.161138) →
265 pairs (best `f21+f22` 1.163123). Improvement +0.171% < the owner's 0.2%
epsilon → stop. Not degenerate: 128/265 pairs beat their parents — pairs help,
just under the "worth another 13k jobs" bar. Pareto note for hardware costing:
`f21` alone delivers 98.3% of the total Hermes gain with a single 2^10-state
feature; the pair adds a 28-bit delta shift register and one more weight table.

## Owner amendment (2026-07-06): precision-first selection

Post-convergence analysis showed the geomean-IPC objective is nearly blind to
predictor precision in this DRAM configuration (ample bandwidth):

- Across all 265 pairs, a **21-point precision spread** (57.9→79.2%) maps to a
  **0.55% IPC spread**; suite-wide corr(IPC, precision) = **+0.36**.
- On the bandwidth-bound traces the correlation is nearly perfect:
  **+0.975** (fotonik3d), **+0.954** (graph500 bfs-3313B); on bfs-3313B
  *recall* correlates **−0.96** — extra predictions cost bandwidth even when
  correct. (bfs-1217B is fine: uniformly 94% precision, Hermes +1.4% there.)
- **0/265 pairs exceeded the best singleton's precision** (f21, 79.5%): at
  rule-scaled thresholds, added features buy recall, not precision. Precision
  is set by the threshold, not the feature count.

The owner therefore amended the final selection criterion to
**precision-first with a justifiable recall trade-off**, judged on
threshold-swept precision-recall curves. Phase-2 sweeps (below) implement it.
Reassuringly, `f21+f22` is precision-rank #2 of 265 (79.2% max) with the best
recall among the precision top-4 — the winner is robust to the criterion change.

## Window-fidelity verdict (50M+200M vs 100M+500M, all 56 singletons)

Aggregate: Spearman **0.784**, top-10 overlap **7/10**. The 0.784 tripped the
pre-set 0.8 tripwire, so it was diagnosed rather than waved through:

- `f21` is **#1 in both windows**, by IPC *and* by precision.
- All 5 beam picks stay in the full-window top-8 (#1→#1, #2→#6, #3→#2,
  #4→#8, #5→#3); the full-window top-5 consists of the same feature families.
- **Precision fidelity is near-perfect: Pearson 0.991, top-10 overlap 8/10.**
- Value-level geo Pearson 0.806 vs rank 0.784: the rank metric is shredded by
  near-ties (most of the 56 sit in a 0.3% IPC band whose internal order is
  noise in ANY window), not by structural disagreement.

**Ruling (operator judgment, flagged for owner review): the short window is
validated for every decision this campaign made** — top-group selection and
precision ordering — with the standing caution that IPC orderings within
±0.1% bands are never meaningful. Recommended closing check: rerun the chosen
final operating point at the full window on the full tlist (~150–450 jobs)
once the owner picks it.

## Key findings (thesis material)

1. **Page-history features dominate at the physical-address uncore**: the
   miss-ratio pair (f21) and spatial footprint (f20) anchor every top
   configuration. Address-identity features (Page/Addr) and the classic
   core-mode feature `f9` (Offset_FirstAccess) do not make the top-20 under
   Pythia — the prefetcher absorbs the loads those features predicted.
2. **Features buy recall; thresholds buy precision.** Every f21-anchored pair
   lost 0.3–2.2pp precision and gained 4.6–5.5pp recall vs f21 alone.
3. **Prediction quality is no longer the bottleneck at the uncore** — recall
   rose 75.8→81.3 at equal precision while IPC moved +0.17%: DDRP's
   latency-saving headroom under a strong prefetcher is the limiter.
4. **Bandwidth sensitivity concentrates in two traces** (fotonik3d,
   graph500-3313B), where precision correlates ~+0.95 with performance —
   the empirical motivation for the precision-first criterion, and the
   canaries to watch in every future table.
5. Region features carry real but subordinate signal (only ever as the second
   feature; granularity 2KB–2MB barely differentiates).

## Operations summary

- **25,680 jobs** across 6 batches (r01 5800 + retry 156; r02 13250 + retry
  270; sweep01 1500; val01 4704), zero data loss: every scored round used
  50/50 (or 147/147) traces after own-health filtering + one retry pass.
- Incidents handled autonomously: 8h backfill starvation (24h default
  walltime; fixed via scontrol right-sizing), FIFO starvation of the tuning
  tail behind twin jobs (fixed by deliberate Nice=200 on the diagnostic
  class), 71 + 64 walltime TIMEOUTs (retried once at higher limits; walltime
  now scales with feature count: 4h floor singletons, 6h pairs/triples),
  NFS-vs-rollup sibling-filter contamination (own-health Filter
  reconstruction, documented in decisions).
- Budget: 25,680 / 120,000 authorized jobs.

## In flight & queued (state.json is live truth)

- **sweep01** (running): 12-point activation-threshold curve for f21+f22;
  probe triples +f20/+f18/+f27@17/+f30@20; 6-point f20+f21 — P-R curve
  judgment per the Phase-2 decision rule.
- **val01** (running): the same sweep on the full 147-trace tlist — direct
  50-vs-147-trace generalization check of the curves.
- Queued (owner vet before launch): pos/neg train-threshold sweep at the
  chosen operating point; weight-table-size sweep (hardware budget); proposed
  finalist full-window confirmation.
