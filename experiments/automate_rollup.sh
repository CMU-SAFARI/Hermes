#!/bin/bash

ROLLUP_SCRIPT=$HERMES_HOME/scripts/rollup.pl

echo "Rolling up statistics for accuracy-coverage comparison (Fig. 9)..."
cd outputs/
$ROLLUP_SCRIPT --tlist ../MICRO22_AE.tlist --exp ../rollup_cov_acc.exp --mfile ../rollup_cov_acc.mfile > ../rollup_cov_acc.csv
cd -

echo "Rolling up statistics for perf. comparision of Hermes, Pythia, and Pythia+Hermes (Fig. 12)..."
cd outputs/
$ROLLUP_SCRIPT --tlist ../MICRO22_AE.tlist --exp ../rollup_perf_hermes.exp --mfile ../rollup_perf.mfile > ../rollup_perf_hermes.csv
cd -

echo "Rolling up statistics for perf. comparison of Pythia, Pythia+HMP, Pythia+TTP, and Pythia+Hermes (Fig. 14)..."
cd outputs/
$ROLLUP_SCRIPT --tlist ../MICRO22_AE.tlist --exp ../rollup_perf_hermes_hmp_ttp.exp --mfile ../rollup_perf.mfile > ../rollup_perf_hermes_hmp_ttp.csv
cd -

echo "Rolling up statistics for perf. comparison of Hermes with varying underlying prefetchers (Fig. 17b)..."
cd outputs/
$ROLLUP_SCRIPT --tlist ../MICRO22_AE.tlist --exp ../rollup_perf_varying_prefetcher.exp --mfile ../rollup_perf.mfile > ../rollup_perf_varying_prefetcher.csv
cd -
