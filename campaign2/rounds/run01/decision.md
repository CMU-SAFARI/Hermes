# Campaign-2 run01 — quad-core speedup table — 2026-07-10

100 random 4-trace mixes x 8 experiments; 25M+75M per core; 4-core/1-channel;
fixed instruction windows (every core measures exactly [W, W+S]).
Metric: per-mix geomean-IPC vs the same mix's c2_nopf; final = geomean over
mixes. ALL 800/800 (mix,exp) pairs valid — no missing data.
All Hermes rows include Pythia (the campaign-1 operating configuration).

| config | vs nopf | vs Pythia | pooled prec% | pooled recall% |
|--------|---------|-----------|--------------|----------------|
| Pythia alone | **1.0689** | 1.0000 | — | — |
| Lite + Pythia (embed on) | 1.0638 | 0.9953 | 84.0 | 66.4 |
| Normal + Pythia (embed on) | 1.0636 | 0.9951 | 84.8 | 67.4 |
| Big + Pythia (embed on) | 1.0633 | 0.9948 | 85.1 | 67.6 |
| Big + Pythia (embed OFF) | 1.0620 | 0.9936 | 83.8 | 66.4 |
| Normal + Pythia (embed OFF) | 1.0616 | 0.9932 | 83.3 | 65.6 |
| Lite + Pythia (embed OFF) | 1.0598 | 0.9915 | 82.4 | 64.1 |

(Pooled P/R = shared-predictor aggregate, full-phase semantics — trend
signal only, per the owner amendment.)

## Finding 1 — THE headline: DDRP inverts under bandwidth contention.

At single core (projection, full suites) Hermes adds +0.4..0.5% on top of
Pythia. At 4 cores sharing ONE DRAM channel, it costs −0.5%: the
speculative direct-DRAM fetches (at 84-85% precision -> ~15% wasted DRAM
transactions, plus redundant fetch pairs) consume bandwidth that
co-runners needed. This is the owner's own campaign-1 hypothesis —
"ample bandwidth masks precision" — confirmed from the other side:
when bandwidth is scarce, precision isn't masked, it's punished.
Consequence for the roadmap: DDRP needs BANDWIDTH-AWARENESS (e.g., gate
on the existing DRAM bw-quartile monitor) and/or a higher activation
threshold at the uncore in multi-core — a concrete, well-scoped
follow-up campaign.

## Finding 2 — the cpu-id embedding works.

Embed on vs off, same version: Lite +0.38pp speedup / +1.6pp precision /
+2.3pp recall; Normal +0.19pp / +1.5pp / +1.8pp; Big +0.12pp / +1.3pp /
+1.2pp. Cross-core weight-table interference is real and the embedding
mitigates it — most for the smallest tables (Lite), exactly as designed.
Embed=on should be the default for all shared-uncore configurations.

## Finding 3 — Big-vs-Lite at multi-core: no performance gap (owner
predicted Big wins).

Performance is flat across versions (1.0633..1.0638 — spread 0.05%,
noise-level), while prediction quality keeps its ladder (84.0/66.4 ->
85.1/67.6). Consistent with E4's page-buffer-degrades-gracefully result:
capacity contention at 4 cores does not turn into IPC. With DDRP
currently value-negative at 4c/1ch, better prediction cannot pay — worth
re-testing the version gap AFTER bandwidth-aware DDRP lands.

## Provenance

run01 survivors (20 mixes) + run01b (80 mixes, deadlock-fixed binary
a327a7cb) + run01c (24 heavy-mix 14h retries) -> 800/800. Incident: 606
run01 jobs died to the warmup-barrier deadlock false-positive (fixed
0c4a1fa, validated at scale — zero fires across run01b/run01c). The fix
gates only the abort heuristic; survivor data is semantically identical.
Scores: rounds/run01/run01_scores.csv; merged stats preserved in the
session scratchpad and the batch dirs.
