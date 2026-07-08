# Campaign1 round e1 rollup

- traces scored: 145 / dropped: 1 (765.roms_r-benchmark2-985B)
- configs scored: 18

## Ranking (tuning window, geomean IPC speedup)

| # | key | vs nopf | vs pythia | worst vs pythia (trace) | prec | recall | flag |
|---|-----|---------|-----------|-------------------------|------|--------|------|
| 1 | f20+f21+f22|act=2|tr=24/-14 | 1.1745 | 1.0197 | 0.933 (800.pot3d_s-pot3d_n1-1519B) | 81.464 | 79.752 | FLAG |
| 2 | f20+f21+f22|act=2|tr=8/-7 | 1.1742 | 1.0194 | 0.932 (800.pot3d_s-pot3d_n1-1519B) | 83.386 | 78.039 | FLAG |
| 3 | f20+f21+f22|act=2|tr=16/-14 | 1.1740 | 1.0193 | 0.946 (800.pot3d_s-pot3d_n1-1519B) | 82.897 | 78.285 | FLAG |
| 4 | f20+f21+f22|act=2|tr=16/-21 | 1.1737 | 1.0190 | 0.946 (800.pot3d_s-pot3d_n1-1519B) | 84.316 | 75.412 | FLAG |
| 5 | f20+f21+f22|act=2|tr=32/-21 | 1.1737 | 1.0190 | 0.953 (865.roms_s-benchmark3-2540B) | 81.512 | 78.732 | FLAG |
| 6 | f20+f21+f22|act=2|tr=16/-7 | 1.1736 | 1.0190 | 0.930 (800.pot3d_s-pot3d_n1-1519B) | 80.918 | 81.727 | FLAG |
| 7 | f20+f21+f22|act=2|tr=8/-21 | 1.1735 | 1.0189 | 0.952 (800.pot3d_s-pot3d_n1-1519B) | 86.303 | 71.443 | FLAG |
| 8 | f20+f21+f22|act=2|tr=8/-14 | 1.1735 | 1.0189 | 0.946 (800.pot3d_s-pot3d_n1-1519B) | 85.206 | 74.294 | FLAG |
| 9 | f20+f21+f22|act=2|tr=32/-28 | 1.1731 | 1.0185 | 0.936 (800.pot3d_s-pot3d_n1-1519B) | 82.788 | 75.103 | FLAG |
| 10 | f20+f21+f22|act=8|tr=16/-21 | 1.1731 | 1.0185 | 0.948 (800.pot3d_s-pot3d_n1-1519B) | 86.798 | 68.963 | FLAG |
| 11 | f20+f21+f22|act=2|tr=24/-7 | 1.1730 | 1.0185 | 0.924 (800.pot3d_s-pot3d_n1-1519B) | 79.359 | 83.086 | FLAG |
| 12 | f20+f21+f22|act=2|tr=32/-14 | 1.1727 | 1.0182 | 0.932 (800.pot3d_s-pot3d_n1-1519B) | 80.045 | 81.120 | FLAG |
| 13 | f20+f21+f22|act=2|tr=24/-28 | 1.1727 | 1.0182 | 0.938 (800.pot3d_s-pot3d_n1-1519B) | 84.056 | 73.782 | FLAG |
| 14 | f20+f21+f22|act=2|tr=32/-7 | 1.1726 | 1.0181 | 0.917 (800.pot3d_s-pot3d_n1-1519B) | 77.908 | 84.245 | FLAG |
| 15 | f20+f21+f22|act=2|tr=16/-28 | 1.1726 | 1.0181 | 0.957 (800.pot3d_s-pot3d_n1-2026B) | 85.261 | 71.822 | FLAG |
| 16 | f20+f21+f22|act=2|tr=8/-28 | 1.1721 | 1.0176 | 0.952 (800.pot3d_s-pot3d_n1-1519B) | 87.157 | 67.634 | FLAG |
| 17 | f20+f21+f22|act=8|tr=8/-21 | 1.1715 | 1.0171 | 0.957 (800.pot3d_s-pot3d_n1-1519B) | 89.717 | 59.710 | FLAG |
| 18 | f20+f21+f22|act=8|tr=8/-7 | 1.1712 | 1.0169 | 0.949 (800.pot3d_s-pot3d_n1-1519B) | 88.294 | 65.224 | FLAG |

- flagged configs (worst-trace < 0.98 vs pythia): 18

## Default top-5 (pending judgment in decision.md)

1. `f20+f21+f22|act=2|tr=24/-14`  geo_nopf=1.1745
2. `f20+f21+f22|act=2|tr=8/-7`  geo_nopf=1.1742
3. `f20+f21+f22|act=2|tr=16/-14`  geo_nopf=1.1740
4. `f20+f21+f22|act=2|tr=16/-21`  geo_nopf=1.1737
5. `f20+f21+f22|act=2|tr=32/-21`  geo_nopf=1.1737
