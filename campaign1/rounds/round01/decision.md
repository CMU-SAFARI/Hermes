# Round 1 decision — 2026-07-05 ~03:00 UTC (autonomous)

## Beam selected (accepting default top-5)

| # | key | geo vs nopf | geo vs pythia | family |
|---|-----|-------------|---------------|--------|
| 1 | f21 | 1.1611 | 1.0201 | PageMissRatio (prime) |
| 2 | f27@21 | 1.1610 | 1.0200 | RegionID×PageMissRatio @2MB |
| 3 | f20 | 1.1601 | 1.0192 | PageSpatialFootprint (prime) |
| 4 | f27@17 | 1.1598 | 1.0189 | RegionID×PageMissRatio @128KB |
| 5 | f18 | 1.1595 | 1.0187 | PageReuseCount (prime) |

Rationale: three distinct feature families in the beam — no sibling
degeneracy, no children-per-parent cap needed. #5 (f18, 1.15952) vs #6
(f26@20, 1.15945) is a statistical tie; kept the prime (simpler, higher).

## Observations (for the thesis)

- Page-history features (miss-ratio pair, spatial footprint, reuse count)
  dominate; the page-buffer state is the signal at the uncore.
- f9 (Offset_FirstAccess) — the strongest single feature in core-mode work —
  is NOT in the top 20 under Pythia + physical uncore. The prefetcher
  changes which loads survive to the LLC, and first-access/offset signal
  appears to be largely absorbed.
- RegionID×PageMissRatio works at 2MB and 128KB (ranks 2/4); pure RegionID
  is mid-pack (~#19). Composites carry the region signal, marginals do not.
- Address-identity memorizers (f2 Page, f3 Addr) mid-pack or below despite
  64k-entry tables — no unfair-advantage problem to worry about.
- Score spread across all 56 singletons is only ~0.3pp (1.1577–1.1611) —
  exactly the near-tie regime beam-5 was designed for.

## Flags

All 56 configs flagged on worst-trace: 749.fotonik3d_r (0.93–0.96 vs
pythia-alone) and occasionally graph500. This is a UNIVERSAL DDRP cost
(speculative DRAM fetches burn bandwidth on this stencil), independent of
feature choice — it cannot discriminate configs in round 1. Watch: do
higher-precision multi-feature sets shrink the fotonik3d loss in round 2+?

## Operational record

- Pipelining: round scored on the tuning window only; the _wf fidelity twins
  (round-1-only) still run behind a deliberate Nice=200; fidelity section
  will be appended to report.md when they drain. Round 2 launches now —
  fidelity is a diagnostic, not a gate (a bad verdict triggers HOLD/re-plan).
- Failures: 71/2900 tuning rows were TIMEOUTs, self-inflicted by trimming
  walltime 4h->3h (slowest trace needs ~3h05m). All 71 retried at 5h in
  batch 20260704T204013Z_campaign1_r01_retry: 156/156 valid. Final scoring
  set: 50/50 traces complete, zero drops. Walltime floor is now 4h (RESUME).
- Scoring input: stats_merged.csv (own-health Filter reconstruction — the
  infra sibling-filter counts still-running twins as failed siblings, which
  would have zeroed 41 healthy traces; validity = own-run health per the
  protocol's intent).

## Round 2

Beam of 5 singletons -> 265 pair configs (275 extensions − 10 cross-member
duplicates), 13,250 jobs at N=2 thresholds (-7/+16/-14), walltime 4h, no
nice. Budget after launch: 19,206 / 120,000.
