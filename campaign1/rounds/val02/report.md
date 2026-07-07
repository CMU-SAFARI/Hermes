# Campaign1 round val02 rollup

- traces scored: 147 / dropped: 0
- configs scored: 9

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f20+f21+f22|act=0 | 1.1611 | 1.0159 | 0.954 (865.roms_s-benchmark3-2540B) | 82.292 | 77.599 | FLAG |
| 2 | f20+f21+f22|act=8 | 1.1609 | 1.0157 | 0.964 (865.roms_s-benchmark3-2540B) | 85.208 | 71.163 | FLAG |
| 3 | f20+f21+f22|act=-2 | 1.1607 | 1.0156 | 0.951 (865.roms_s-benchmark3-2540B) | 81.378 | 78.742 | FLAG |
| 4 | f20+f21+f22|act=4 | 1.1607 | 1.0155 | 0.957 (865.roms_s-benchmark3-2540B) | 83.775 | 74.584 | FLAG |
| 5 | f20+f21+f22|act=6 | 1.1605 | 1.0154 | 0.959 (865.roms_s-benchmark3-2540B) | 84.462 | 72.788 | FLAG |
| 6 | f20+f21|act=11 | 1.1602 | 1.0151 | 0.963 (865.roms_s-benchmark3-2540B) | 85.054 | 68.588 | FLAG |
| 7 | f20+f21+f22|act=10 | 1.1598 | 1.0148 | 0.961 (865.roms_s-benchmark3-2540B) | 85.994 | 68.628 | FLAG |
| 8 | f21+f22|act=9 | 1.1597 | 1.0146 | 0.963 (865.roms_s-benchmark3-2540B) | 84.106 | 69.391 | FLAG |
| 9 | f21+f22|act=11 | 1.1591 | 1.0141 | 0.963 (800.pot3d_s-pot3d_n1-2544B) | 85.002 | 66.827 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 9

## Default top-5 (pending judgment in decision.md)

1. `f20+f21+f22|act=0`  geo_nopf=1.1611
2. `f20+f21+f22|act=8`  geo_nopf=1.1609
3. `f20+f21+f22|act=-2`  geo_nopf=1.1607
4. `f20+f21+f22|act=4`  geo_nopf=1.1607
5. `f20+f21+f22|act=6`  geo_nopf=1.1605
