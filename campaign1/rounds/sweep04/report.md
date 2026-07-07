# Campaign1 round sweep04 rollup

- traces scored: 50 / dropped: 0
- configs scored: 12

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f20+f21+f22|act=2|w=65536x16384x65536 | 1.1633 | 1.0220 | 0.974 (854.graph500_s-bfs-3313B) | 83.944 | 76.340 | FLAG |
| 2 | f20+f21+f22|act=2|w=65536x65536x1024 | 1.1629 | 1.0216 | 0.974 (854.graph500_s-bfs-3313B) | 83.267 | 76.046 | FLAG |
| 3 | f20+f21+f22|act=2|w=1024x65536x65536 | 1.1628 | 1.0215 | 0.973 (854.graph500_s-bfs-3313B) | 83.362 | 75.885 | FLAG |
| 4 | f20+f21+f22|act=2|w=16384x16384x16384 | 1.1621 | 1.0209 | 0.973 (854.graph500_s-bfs-3313B) | 83.789 | 76.153 | FLAG |
| 5 | f20+f21+f22|act=2|w=1024x1024x1024 | 1.1619 | 1.0207 | 0.973 (854.graph500_s-bfs-3313B) | 82.476 | 74.828 | FLAG |
| 6 | f20+f21+f22|act=2|w=65536x4096x65536 | 1.1619 | 1.0207 | 0.974 (854.graph500_s-bfs-3313B) | 83.894 | 76.221 | FLAG |
| 7 | f20+f21+f22|act=2|w=65536x65536x16384 | 1.1615 | 1.0204 | 0.974 (854.graph500_s-bfs-3313B) | 83.885 | 76.206 | FLAG |
| 8 | f20+f21+f22|act=2|w=4096x65536x65536 | 1.1614 | 1.0203 | 0.973 (854.graph500_s-bfs-3313B) | 83.593 | 75.992 | FLAG |
| 9 | f20+f21+f22|act=2|w=4096x4096x4096 | 1.1611 | 1.0200 | 0.972 (854.graph500_s-bfs-3313B) | 83.300 | 75.629 | FLAG |
| 10 | f20+f21+f22|act=2|w=65536x65536x4096 | 1.1609 | 1.0199 | 0.972 (800.pot3d_s-pot3d_n1-1519B) | 83.604 | 76.058 | FLAG |
| 11 | f20+f21+f22|act=2|w=16384x65536x65536 | 1.1609 | 1.0199 | 0.971 (800.pot3d_s-pot3d_n1-1519B) | 83.682 | 75.921 | FLAG |
| 12 | f20+f21+f22|act=2|w=65536x1024x65536 | 1.1608 | 1.0198 | 0.970 (800.pot3d_s-pot3d_n1-1519B) | 83.789 | 75.478 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 12

## Default top-5 (pending judgment in decision.md)

1. `f20+f21+f22|act=2|w=65536x16384x65536`  geo_nopf=1.1633
2. `f20+f21+f22|act=2|w=65536x65536x1024`  geo_nopf=1.1629
3. `f20+f21+f22|act=2|w=1024x65536x65536`  geo_nopf=1.1628
4. `f20+f21+f22|act=2|w=16384x16384x16384`  geo_nopf=1.1621
5. `f20+f21+f22|act=2|w=1024x1024x1024`  geo_nopf=1.1619
