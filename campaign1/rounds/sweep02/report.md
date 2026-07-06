# Campaign1 round sweep02 rollup

- traces scored: 50 / dropped: 0
- configs scored: 9

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f20+f21+f22|act=8 | 1.1625 | 1.0213 | 0.978 (854.graph500_s-bfs-3313B) | 85.826 | 72.028 | FLAG |
| 2 | f20+f21+f22|act=0 | 1.1619 | 1.0207 | 0.972 (854.graph500_s-bfs-3313B) | 83.288 | 77.603 | FLAG |
| 3 | f20+f21+f22|act=4 | 1.1613 | 1.0202 | 0.976 (854.graph500_s-bfs-3313B) | 84.513 | 74.896 | FLAG |
| 4 | f20+f21+f22|act=-2 | 1.1606 | 1.0196 | 0.969 (854.graph500_s-bfs-3313B) | 82.396 | 78.586 | FLAG |
| 5 | f20+f21+f22|act=6 | 1.1606 | 1.0196 | 0.977 (854.graph500_s-bfs-3313B) | 85.112 | 73.380 | FLAG |
| 6 | f20+f21|act=11 | 1.1598 | 1.0189 | 0.980 (854.graph500_s-bfs-3313B) | 85.521 | 69.755 |  |
| 7 | f21+f22|act=9 | 1.1597 | 1.0188 | 0.978 (749.fotonik3d_r-default-599B) | 84.733 | 70.158 | FLAG |
| 8 | f20+f21+f22|act=10 | 1.1595 | 1.0186 | 0.978 (800.pot3d_s-pot3d_n1-1519B) | 86.388 | 69.719 | FLAG |
| 9 | f21+f22|act=11 | 1.1594 | 1.0186 | 0.979 (749.fotonik3d_r-default-599B) | 85.401 | 67.913 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 8

## Default top-5 (pending judgment in decision.md)

1. `f20+f21+f22|act=8`  geo_nopf=1.1625
2. `f20+f21+f22|act=0`  geo_nopf=1.1619
3. `f20+f21+f22|act=4`  geo_nopf=1.1613
4. `f20+f21+f22|act=-2`  geo_nopf=1.1606
5. `f20+f21+f22|act=6`  geo_nopf=1.1606
