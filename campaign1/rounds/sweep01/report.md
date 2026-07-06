# Campaign1 round sweep01 rollup

- traces scored: 50 / dropped: 0
- configs scored: 30

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f20+f21+f22|act=2 | 1.1636 | 1.0223 | 0.974 (854.graph500_s-bfs-3313B) | 84.000 | 76.454 | FLAG |
| 2 | f21+f22|act=-5 | 1.1632 | 1.0219 | 0.955 (749.fotonik3d_r-default-599B) | 79.836 | 80.135 | FLAG |
| 3 | f21+f22|act=-7 | 1.1631 | 1.0218 | 0.952 (749.fotonik3d_r-default-599B) | 78.762 | 81.312 | FLAG |
| 4 | f20+f21+f22|act=-10 | 1.1630 | 1.0218 | 0.963 (854.graph500_s-bfs-3313B) | 78.800 | 83.024 | FLAG |
| 5 | f21+f22+f27@17|act=-4 | 1.1629 | 1.0216 | 0.962 (749.fotonik3d_r-default-599B) | 80.603 | 78.715 | FLAG |
| 6 | f18+f21+f22|act=-10 | 1.1626 | 1.0214 | 0.957 (749.fotonik3d_r-default-599B) | 78.308 | 81.393 | FLAG |
| 7 | f21+f22+f27@17|act=2 | 1.1625 | 1.0213 | 0.971 (749.fotonik3d_r-default-599B) | 82.981 | 75.120 | FLAG |
| 8 | f21+f22+f30@20|act=2 | 1.1622 | 1.0211 | 0.967 (749.fotonik3d_r-default-599B) | 82.633 | 74.886 | FLAG |
| 9 | f18+f21+f22|act=2 | 1.1619 | 1.0208 | 0.965 (749.fotonik3d_r-default-599B) | 82.677 | 75.019 | FLAG |
| 10 | f21+f22+f30@20|act=-10 | 1.1617 | 1.0206 | 0.948 (854.graph500_s-bfs-3313B) | 75.705 | 82.974 | FLAG |
| 11 | f20+f21+f22|act=-4 | 1.1615 | 1.0204 | 0.967 (854.graph500_s-bfs-3313B) | 81.583 | 79.698 | FLAG |
| 12 | f20+f21|act=-5 | 1.1612 | 1.0202 | 0.973 (854.graph500_s-bfs-3313B) | 80.094 | 79.296 | FLAG |
| 13 | f21+f22|act=3 | 1.1609 | 1.0199 | 0.974 (749.fotonik3d_r-default-599B) | 82.836 | 74.566 | FLAG |
| 14 | f20+f21|act=-9 | 1.1608 | 1.0198 | 0.969 (854.graph500_s-bfs-3313B) | 77.949 | 81.938 | FLAG |
| 15 | f21+f22|act=5 | 1.1608 | 1.0198 | 0.977 (749.fotonik3d_r-default-599B) | 83.455 | 73.178 | FLAG |
| 16 | f20+f21|act=-1 | 1.1605 | 1.0196 | 0.957 (800.pot3d_s-pot3d_n1-1519B) | 81.740 | 77.358 | FLAG |
| 17 | f21+f22|act=-1 | 1.1605 | 1.0195 | 0.961 (800.pot3d_s-pot3d_n1-1519B) | 81.371 | 77.733 | FLAG |
| 18 | f21+f22|act=-3 | 1.1604 | 1.0195 | 0.952 (800.pot3d_s-pot3d_n1-1519B) | 80.515 | 78.864 | FLAG |
| 19 | f21+f22|act=-13 | 1.1603 | 1.0194 | 0.936 (749.fotonik3d_r-default-599B) | 69.912 | 87.985 | FLAG |
| 20 | f20+f21|act=3 | 1.1603 | 1.0193 | 0.968 (800.pot3d_s-pot3d_n1-1519B) | 83.059 | 74.929 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 30

## Default top-5 (pending judgment in decision.md)

1. `f20+f21+f22|act=2`  geo_nopf=1.1636
2. `f21+f22|act=-5`  geo_nopf=1.1632
3. `f21+f22|act=-7`  geo_nopf=1.1631
4. `f20+f21+f22|act=-10`  geo_nopf=1.1630
5. `f21+f22+f27@17|act=-4`  geo_nopf=1.1629
