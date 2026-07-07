# Campaign1 round val03 rollup

- traces scored: 146 / dropped: 1 (853.ns3_s-tcp_validation-202B)
- configs scored: 4

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f20+f21+f22|act=2 | 1.1724 | 1.0190 | 0.938 (800.pot3d_s-pot3d_n1-1519B) | 82.837 | 77.324 | FLAG |
| 2 | f20+f21+f22|act=6 | 1.1715 | 1.0183 | 0.951 (800.pot3d_s-pot3d_n1-1519B) | 84.503 | 73.890 | FLAG |
| 3 | f20+f21+f22|act=8 | 1.1709 | 1.0177 | 0.944 (800.pot3d_s-pot3d_n1-1519B) | 85.154 | 71.905 | FLAG |
| 4 | f21+f22|act=7 | 1.1706 | 1.0175 | 0.941 (800.pot3d_s-pot3d_n1-1519B) | 83.425 | 71.888 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 4

## Default top-5 (pending judgment in decision.md)

1. `f20+f21+f22|act=2`  geo_nopf=1.1724
2. `f20+f21+f22|act=6`  geo_nopf=1.1715
3. `f20+f21+f22|act=8`  geo_nopf=1.1709
4. `f21+f22|act=7`  geo_nopf=1.1706
