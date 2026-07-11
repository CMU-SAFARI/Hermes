# Campaign-2 2-channel verdict — 2026-07-11

Same 100 mixes, 4 cores, TWO DRAM channels (binary 4core-2ch, built
4 2 1). 656/700 valid; scored on n=92 common mixes (FINAL — the 63-job retry
was terminated at owner request; 9 heavy mixes excluded).

## 1ch vs 2ch side-by-side (vs same-channel-count, same-mix baselines)

| config | 1ch vs nopf | 2ch vs nopf | 1ch vs Pythia | 2ch vs Pythia |
|--------|------------:|------------:|--------------:|--------------:|
| Pythia alone | 1.0689 | 1.0772 | 1.0000 | 1.0000 |
| XPT + Pythia | 1.0652 | 1.0743 | 0.9972 | 0.9962 |
| Big + Pythia (embed) | 1.0633 | 1.0727 | 0.9948 | 0.9959 |
| Big ALONE | 1.0238 | 1.0245 | 0.9568 | 0.9510 |
| XPT ALONE | 1.0042 | 1.0051 | 0.9395 | 0.9331 |

## VERDICT: doubling channels does NOT flip Hermes+Pythia positive
## (−0.52% -> −0.41%), and the deep losers barely recover
## (geomean 0.9710 -> 0.9718; 10 of 14 still below −2%).

The naive mapping "2ch/4core = 1600 MTPS per core" — where the
single-core sweep shows +1.4% for Hermes-on-Pythia — fails. Two reasons
the multicore operating point is effectively much poorer than the
nominal division: (1) Pythia itself scales up to consume the added
bandwidth (its own gain 1ch->2ch: +0.83pp), so headroom never
materializes for DDRP; (2) four interleaved streams add bank/row-buffer
interference the single-core curve doesn't model — the deep-loser mixes
are interference-bound, not capacity-bound, which is why doubling
capacity doesn't rescue them.

Practical read: in shared-memory multicore, Hermes's DDRP as-configured
stays a small net tax on top of a strong prefetcher even at 2 channels;
the single-core crossover (~400-800 MTPS) translates to a multicore
crossover that sits beyond realistic channel counts for 4 cores. This
is Athena's problem statement, quantified from the placement-study side.
XPT+Pythia remains statistically tied with Big+Pythia at both channel
counts (abstention vs taxed coverage — same economics as run02's
volume analysis).

Provenance: batch 20260710T133355Z_campaign2_2ch; 44 rows lost to 14h
caps across 9 heavy mixes (retry 2chr at 16h in flight, folds in
quietly); baselines c2ch_nopf/c2ch_pythia mapped for the scorer.
