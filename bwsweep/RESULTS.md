# Bandwidth Sweep — FINAL (5,241/5,256; owner terminated the last cells — slowest bw200/400 traces permanently dropped from those points' common sets)

Single core, 146-trace list, full window; --dram_io_freq sweeps the
single-channel rate; per-point geomeans over the common trace set (n per
row); vs same-point nopf and same-point Pythia. 2026-07-11.

## Speedup vs no-prefetch (geomean)

| MTPS | Pythia | Hermes-Normal ALONE | Normal+Pythia | XPT ALONE | XPT+Pythia |
|------|--------|--------------------:|--------------:|----------:|-----------:|
| 200 | **0.9334** | **1.0013** | 0.9024 | 1.0029 | 0.9254 |
| 400 | 1.0148 | **1.0273** | 1.0046 | 1.0069 | 1.0080 |
| 800 | 1.0808 | 1.0457 | **1.0874** | 1.0110 | 1.0764 |
| 1600 | 1.1299 | 1.0534 | **1.1460** | 1.0133 | 1.1265 |
| 3200 | 1.1505 | 1.0582 | **1.1708** | 1.0144 | 1.1492 |
| 6400 | 1.1533 | 1.0607 | **1.1743** | 1.0152 | 1.1516 |

## vs Pythia (the composition view)

| MTPS | Normal ALONE | Normal+Pythia | XPT+Pythia |
|------|-------------:|--------------:|-----------:|
| 200 | **+7.28%** | −3.32% | −0.85% |
| 400 | **+1.23%** | −1.00% | −0.67% |
| 800 | −3.24% | **+0.61%** | −0.40% |
| 1600 | −6.77% | **+1.42%** | −0.30% |
| 3200 | −8.02% | **+1.76%** | −0.12% |
| 6400 | −8.03% | **+1.82%** | −0.14% |

## The three crossovers

1. **Hermes-alone vs Pythia-alone crosses between 400 and 800 MTPS.**
   Below ~500-600 MTPS effective bandwidth, PREDICTION beats PREFETCHING:
   at 200 MTPS Pythia is 6.7% BELOW no-prefetch (its speculation poisons a
   starved channel) while Hermes-alone holds +0.1% — a 7.3% advantage.
   The owner's hypothesis is confirmed — the 4c/1ch quad result sat at
   ~800 MTPS effective per core, just past the crossover, which is
   exactly why Pythia still won there (consistency check: the 800-MTPS
   row reproduces the quad-core ordering).
2. **Hermes-on-top-of-Pythia turns net-positive between 400 and 800
   MTPS** (−1.0% -> +0.6%), growing to +1.8% at ample bandwidth. The
   3200-MTPS cell independently reproduces campaign-1's full-window
   headline (1.1708 vs nopf — same config, same traces, different
   batch), validating the whole pipeline end to end.
3. **XPT+Pythia never goes positive at ANY point on the axis**
   (−0.9%..−0.1%): too little coverage to add value with headroom, and
   still a small net tax when starved. XPT-alone stays a flat
   +0.3..+1.5% vs nopf — only interesting at <=400 MTPS, where it ties
   Hermes-alone vs nopf (both predictors' TP value survives; XPT's tiny
   recall costs it nothing there because Pythia isn't around to punish).

## Prediction-quality along the axis
Hermes-alone: 95.5p/95.0r flat across all points (bandwidth doesn't
change WHAT misses — only what misses cost). Normal+Pythia: 84-85.5p,
recall drifts 74.7 -> 70.0 as growing bandwidth lets Pythia cover more.
XPT-alone: 97.7p/25.8r flat; XPT+Pythia: ~61p, recall 9.0 -> 5.8.

## Data notes
FINAL common trace sets: n=141 at 200 MTPS, n=144 at 400 (slowest traces'
cells terminated at owner request 2026-07-11 — conclusions unaffected);
other points complete at 146. Batch
20260710T173114Z_bwsweep, rbdev fc49264.
