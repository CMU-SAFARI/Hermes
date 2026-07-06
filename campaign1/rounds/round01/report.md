# Campaign1 round 1 rollup

- traces scored: 50 / dropped: 0
- configs scored: 56

## Window fidelity (50M+200M vs 100M+500M)
- Spearman rank correlation over 56 configs: **0.784**
- top-10 overlap: 7/10

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f21 | 1.1611 | 1.0201 | 0.962 (749.fotonik3d_r-default-599B) | 79.466 | 75.835 | FLAG |
| 2 | f27@21 | 1.1610 | 1.0200 | 0.956 (749.fotonik3d_r-default-599B) | 68.303 | 77.940 | FLAG |
| 3 | f20 | 1.1601 | 1.0192 | 0.963 (854.graph500_s-bfs-3313B) | 71.485 | 75.927 | FLAG |
| 4 | f27@17 | 1.1598 | 1.0189 | 0.946 (749.fotonik3d_r-default-599B) | 61.317 | 76.233 | FLAG |
| 5 | f18 | 1.1595 | 1.0187 | 0.959 (749.fotonik3d_r-default-599B) | 76.774 | 68.626 | FLAG |
| 6 | f26@20 | 1.1595 | 1.0186 | 0.952 (749.fotonik3d_r-default-599B) | 66.712 | 69.811 | FLAG |
| 7 | f26@21 | 1.1593 | 1.0185 | 0.960 (749.fotonik3d_r-default-599B) | 67.853 | 69.795 | FLAG |
| 8 | f19 | 1.1591 | 1.0183 | 0.957 (749.fotonik3d_r-default-599B) | 76.251 | 67.671 | FLAG |
| 9 | f29@20 | 1.1589 | 1.0182 | 0.940 (749.fotonik3d_r-default-599B) | 59.957 | 74.863 | FLAG |
| 10 | f29@21 | 1.1585 | 1.0178 | 0.943 (749.fotonik3d_r-default-599B) | 60.795 | 75.105 | FLAG |
| 11 | f25@14 | 1.1585 | 1.0178 | 0.940 (749.fotonik3d_r-default-599B) | 63.973 | 76.069 | FLAG |
| 12 | f27@20 | 1.1584 | 1.0177 | 0.958 (749.fotonik3d_r-default-599B) | 67.265 | 77.816 | FLAG |
| 13 | f28@20 | 1.1582 | 1.0175 | 0.949 (749.fotonik3d_r-default-599B) | 68.091 | 66.147 | FLAG |
| 14 | f25@20 | 1.1581 | 1.0174 | 0.960 (749.fotonik3d_r-default-599B) | 73.202 | 71.217 | FLAG |
| 15 | f12 | 1.1580 | 1.0173 | 0.934 (749.fotonik3d_r-default-599B) | 69.870 | 59.479 | FLAG |
| 16 | f2 | 1.1579 | 1.0173 | 0.939 (749.fotonik3d_r-default-599B) | 63.961 | 77.255 | FLAG |
| 17 | f26@17 | 1.1579 | 1.0172 | 0.936 (749.fotonik3d_r-default-599B) | 58.891 | 67.749 | FLAG |
| 18 | f30@20 | 1.1578 | 1.0171 | 0.935 (854.graph500_s-bfs-3313B) | 59.292 | 66.621 | FLAG |
| 19 | f23@20 | 1.1577 | 1.0171 | 0.948 (749.fotonik3d_r-default-599B) | 66.681 | 59.602 | FLAG |
| 20 | f23@21 | 1.1577 | 1.0171 | 0.945 (749.fotonik3d_r-default-599B) | 66.594 | 59.458 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 56

## Default top-5 (pending judgment in decision.md)

1. `f21`  geo_nopf=1.1611
2. `f27@21`  geo_nopf=1.1610
3. `f20`  geo_nopf=1.1601
4. `f27@17`  geo_nopf=1.1598
5. `f18`  geo_nopf=1.1595
