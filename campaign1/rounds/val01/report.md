# Campaign1 round val01 rollup

- traces scored: 147 / dropped: 0
- configs scored: 30

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f20+f21+f22|act=2 | 1.1617 | 1.0164 | 0.954 (865.roms_s-benchmark3-2540B) | 83.157 | 76.158 | FLAG |
| 2 | f20+f21+f22|act=-4 | 1.1612 | 1.0160 | 0.950 (865.roms_s-benchmark3-2540B) | 80.489 | 80.095 | FLAG |
| 3 | f18+f21+f22|act=2 | 1.1608 | 1.0156 | 0.959 (865.roms_s-benchmark3-2540B) | 81.752 | 75.006 | FLAG |
| 4 | f20+f21|act=3 | 1.1606 | 1.0155 | 0.963 (865.roms_s-benchmark3-2540B) | 82.283 | 74.416 | FLAG |
| 5 | f20+f21+f22|act=-10 | 1.1604 | 1.0153 | 0.940 (800.pot3d_s-pot3d_n1-1067B) | 77.276 | 83.624 | FLAG |
| 6 | f21+f22+f30@20|act=2 | 1.1603 | 1.0152 | 0.946 (800.pot3d_s-pot3d_n1-2544B) | 81.629 | 74.952 | FLAG |
| 7 | f21+f22+f27@17|act=-4 | 1.1603 | 1.0152 | 0.948 (865.roms_s-benchmark3-2540B) | 79.303 | 79.227 | FLAG |
| 8 | f20+f21|act=7 | 1.1601 | 1.0150 | 0.959 (865.roms_s-benchmark3-2540B) | 83.500 | 71.891 | FLAG |
| 9 | f21+f22|act=3 | 1.1601 | 1.0150 | 0.957 (865.roms_s-benchmark3-2540B) | 81.957 | 74.279 | FLAG |
| 10 | f18+f21+f22|act=-10 | 1.1601 | 1.0150 | 0.944 (865.roms_s-benchmark3-2540B) | 76.796 | 81.953 | FLAG |
| 11 | f21+f22|act=-7 | 1.1600 | 1.0149 | 0.928 (800.pot3d_s-pot3d_n1-1067B) | 77.165 | 81.899 | FLAG |
| 12 | f18+f21+f22|act=-4 | 1.1600 | 1.0149 | 0.950 (800.pot3d_s-pot3d_n1-2544B) | 79.510 | 78.483 | FLAG |
| 13 | f21+f22|act=-3 | 1.1600 | 1.0149 | 0.947 (800.pot3d_s-pot3d_n1-1067B) | 79.351 | 79.251 | FLAG |
| 14 | f21+f22|act=1 | 1.1600 | 1.0149 | 0.954 (865.roms_s-benchmark3-2540B) | 81.160 | 75.771 | FLAG |
| 15 | f20+f21|act=-1 | 1.1600 | 1.0149 | 0.956 (865.roms_s-benchmark3-2540B) | 80.737 | 77.267 | FLAG |
| 16 | f21+f22+f27@17|act=2 | 1.1600 | 1.0149 | 0.943 (800.pot3d_s-pot3d_n1-2544B) | 82.093 | 75.225 | FLAG |
| 17 | f21+f22|act=5 | 1.1597 | 1.0147 | 0.960 (800.pot3d_s-pot3d_n1-2544B) | 82.618 | 72.638 | FLAG |
| 18 | f21+f22|act=7 | 1.1597 | 1.0147 | 0.959 (865.roms_s-benchmark3-2540B) | 83.392 | 71.237 | FLAG |
| 19 | f21+f22|act=-1 | 1.1597 | 1.0147 | 0.947 (800.pot3d_s-pot3d_n1-2544B) | 80.266 | 77.991 | FLAG |
| 20 | f21+f22|act=-5 | 1.1597 | 1.0146 | 0.936 (800.pot3d_s-pot3d_n1-1067B) | 78.338 | 80.516 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 30

## Default top-5 (pending judgment in decision.md)

1. `f20+f21+f22|act=2`  geo_nopf=1.1617
2. `f20+f21+f22|act=-4`  geo_nopf=1.1612
3. `f18+f21+f22|act=2`  geo_nopf=1.1608
4. `f20+f21|act=3`  geo_nopf=1.1606
5. `f20+f21+f22|act=-10`  geo_nopf=1.1604
