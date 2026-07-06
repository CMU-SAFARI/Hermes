# Campaign1 round 2 rollup

- traces scored: 50 / dropped: 0
- configs scored: 265

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f21+f22 | 1.1631 | 1.0218 | 0.952 (749.fotonik3d_r-default-599B) | 78.762 | 81.312 | FLAG |
| 2 | f20+f27@17 | 1.1629 | 1.0216 | 0.965 (854.graph500_s-bfs-3313B) | 73.084 | 81.654 | FLAG |
| 3 | f2+f20 | 1.1625 | 1.0213 | 0.971 (749.fotonik3d_r-default-599B) | 74.947 | 82.645 | FLAG |
| 4 | f20+f30@21 | 1.1625 | 1.0213 | 0.962 (854.graph500_s-bfs-3313B) | 71.882 | 83.226 | FLAG |
| 5 | f20+f30@11 | 1.1623 | 1.0211 | 0.962 (854.graph500_s-bfs-3313B) | 70.598 | 83.201 | FLAG |
| 6 | f20+f25@20 | 1.1621 | 1.0210 | 0.963 (854.graph500_s-bfs-3313B) | 75.916 | 79.506 | FLAG |
| 7 | f20+f29@21 | 1.1621 | 1.0210 | 0.962 (854.graph500_s-bfs-3313B) | 69.723 | 80.942 | FLAG |
| 8 | f12+f20 | 1.1621 | 1.0209 | 0.962 (854.graph500_s-bfs-3313B) | 76.828 | 78.443 | FLAG |
| 9 | f21+f23@12 | 1.1620 | 1.0209 | 0.952 (749.fotonik3d_r-default-599B) | 77.565 | 81.052 | FLAG |
| 10 | f18+f21 | 1.1620 | 1.0209 | 0.962 (749.fotonik3d_r-default-599B) | 78.344 | 78.673 | FLAG |
| 11 | f10+f20 | 1.1620 | 1.0208 | 0.962 (854.graph500_s-bfs-3313B) | 77.006 | 78.495 | FLAG |
| 12 | f21+f30@21 | 1.1620 | 1.0208 | 0.958 (749.fotonik3d_r-default-599B) | 78.191 | 80.433 | FLAG |
| 13 | f21+f30@20 | 1.1620 | 1.0208 | 0.958 (749.fotonik3d_r-default-599B) | 78.439 | 80.462 | FLAG |
| 14 | f20+f26@20 | 1.1620 | 1.0208 | 0.964 (854.graph500_s-bfs-3313B) | 74.230 | 76.339 | FLAG |
| 15 | f21+f26@12 | 1.1620 | 1.0208 | 0.958 (749.fotonik3d_r-default-599B) | 78.043 | 79.286 | FLAG |
| 16 | f21+f28@14 | 1.1619 | 1.0208 | 0.957 (749.fotonik3d_r-default-599B) | 77.578 | 80.327 | FLAG |
| 17 | f21+f29@20 | 1.1619 | 1.0208 | 0.954 (749.fotonik3d_r-default-599B) | 78.238 | 79.685 | FLAG |
| 18 | f21+f23@11 | 1.1619 | 1.0208 | 0.955 (749.fotonik3d_r-default-599B) | 77.506 | 80.808 | FLAG |
| 19 | f2+f27@21 | 1.1619 | 1.0208 | 0.946 (749.fotonik3d_r-default-599B) | 69.189 | 84.330 | FLAG |
| 20 | f21+f28@12 | 1.1619 | 1.0208 | 0.960 (749.fotonik3d_r-default-599B) | 77.827 | 80.302 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 265

## Default top-5 (pending judgment in decision.md)

1. `f21+f22`  geo_nopf=1.1631
2. `f20+f27@17`  geo_nopf=1.1629
3. `f2+f20`  geo_nopf=1.1625
4. `f20+f30@21`  geo_nopf=1.1625
5. `f20+f30@11`  geo_nopf=1.1623
