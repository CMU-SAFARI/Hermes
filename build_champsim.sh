#!/bin/bash

if [ "$#" -ne 8 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./build_champsim.sh [core_uarch] [l1d_pref] [l2c_pref] [llc_pref] [llc_repl] [num_core] [num_dram_channels] [log_num_dram_channels]"
    exit 1
fi

# some defaults
BRANCH=perceptron   	# fixing branch predictor
L1I_PREFETCHER=no 	    # prefetcher/*.l1i_pref

# ChampSim configuration
CORE_UARCH=$1           # core microarchitecture
L1D_PREFETCHER=$2 	    # prefetcher/*.l1d_pref
L2C_PREFETCHER=$3  	    # prefetcher/*.l2c_pref
LLC_PREFETCHER=$4   	# prefetcher/*.llc_pref
LLC_REPLACEMENT=$5  	# fixing it to SHiP
NUM_CORE=$6         	# tested up to 8-core system
NUM_DRAM_CHANNELS=$7    # number of DRAM channels
LOG_NUM_DRAM_CHANNELS=$8

############## Some useful macros ###############
BOLD=$(tput bold)
NORMAL=$(tput sgr0)
#################################################

################## Sanity check #################
if [ ! -f ./inc/uarch/${CORE_UARCH}.h ]; then
    echo "[ERROR] Cannot find core architecture definition ${CORE_UARCH}"
	echo "[ERROR] Possible core architectures from inc/arch/*.h"
    find ./inc/uarch -name "*.h"
    exit 1
fi

if [ ! -f ./branch/${BRANCH}.bpred ]; then
    echo "[ERROR] Cannot find branch predictor"
	echo "[ERROR] Possible branch predictors from branch/*.bpred "
    find branch -name "*.bpred"
    exit 1
fi

if [ ! -f ./prefetcher/${L1I_PREFETCHER}.l1i_pref ]; then
    echo "[ERROR] Cannot find L1I prefetcher"
	echo "[ERROR] Possible L1I prefetchers from prefetcher/*.l1i_pref "
    find prefetcher -name "*.l1i_pref"
    exit 1
fi

if [ ! -f ./prefetcher/${L1D_PREFETCHER}.l1d_pref ]; then
    echo "[ERROR] Cannot find L1D prefetcher"
	echo "[ERROR] Possible L1D prefetchers from prefetcher/*.l1d_pref "
    find prefetcher -name "*.l1d_pref"
    exit 1
fi

if [ ! -f ./prefetcher/${L2C_PREFETCHER}.l2c_pref ]; then
    echo "[ERROR] Cannot find L2C prefetcher"
	echo "[ERROR] Possible L2C prefetchers from prefetcher/*.l2c_pref "
    find prefetcher -name "*.l2c_pref"
    exit 1
fi

if [ ! -f ./prefetcher/${LLC_PREFETCHER}.llc_pref ]; then
    echo "[ERROR] Cannot find LLC prefetcher"
	echo "[ERROR] Possible LLC prefetchers from prefetcher/*.llc_pref "
    find prefetcher -name "*.llc_pref"
    exit 1
fi

if [ ! -f ./replacement/${LLC_REPLACEMENT}.llc_repl ]; then
    echo "[ERROR] Cannot find LLC replacement policy"
	echo "[ERROR] Possible LLC replacement policy from replacement/*.llc_repl"
    find replacement -name "*.llc_repl"
    exit 1
fi

# Check num_core
re='^[0-9]+$'
if ! [[ $NUM_CORE =~ $re ]] ; then
    echo "[ERROR]: num_core is NOT a number" >&2;
    exit 1
fi
#################################################

# Change core microarchitecture definition file
cp inc/uarch/${CORE_UARCH}.h inc/defs.h

# Check for multi-core
if [ "$NUM_CORE" -gt "1" ]; then
    echo "Building ChampSim for ${CORE_UARCH} ${NUM_CORE}-core configuration with $NUM_DRAM_CHANNELS DRAM channels..."
    sed -i.bak 's/\<NUM_CPUS 1\>/NUM_CPUS '${NUM_CORE}'/g' inc/defs.h
	sed -i.bak 's/\<DRAM_CHANNELS 1\>/DRAM_CHANNELS '${NUM_DRAM_CHANNELS}'/g' inc/defs.h
	sed -i.bak 's/\<LOG2_DRAM_CHANNELS 0\>/LOG2_DRAM_CHANNELS '${LOG_NUM_DRAM_CHANNELS}'/g' inc/defs.h
else
    if [ "$NUM_CORE" -lt "1" ]; then
        echo "Number of core: $NUM_CORE must be greater or equal than 1"
        exit 1
    else
        echo "Building ChampSim for ${CORE_UARCH} ${NUM_CORE}-core configuration with 1 DRAM channels..."
    fi
fi
echo

# Change prefetchers and replacement policy
cp branch/${BRANCH}.bpred branch/branch_predictor.cc
cp prefetcher/${L1I_PREFETCHER}.l1i_pref prefetcher/l1i_prefetcher.cc
cp prefetcher/${L1D_PREFETCHER}.l1d_pref prefetcher/l1d_prefetcher.cc
cp prefetcher/${L2C_PREFETCHER}.l2c_pref prefetcher/l2c_prefetcher.cc
cp prefetcher/${LLC_PREFETCHER}.llc_pref prefetcher/llc_prefetcher.cc
cp replacement/${LLC_REPLACEMENT}.llc_repl replacement/llc_replacement.cc

# Build
mkdir -p bin
rm -f bin/champsim
make clean
make

# Sanity check
echo ""
if [ ! -f bin/champsim ]; then
    echo "${BOLD}ChampSim build FAILED!"
    echo ""
    exit 1
fi

echo "${BOLD}ChampSim is successfully built"
echo "Core uArch: ${CORE_UARCH}"
echo "Branch Predictor: ${BRANCH}"
echo "L1I Prefetcher: ${L1I_PREFETCHER}"
echo "L1D Prefetcher: ${L1D_PREFETCHER}"
echo "L2C Prefetcher: ${L2C_PREFETCHER}"
echo "LLC Prefetcher: ${LLC_PREFETCHER}"
echo "LLC Replacement: ${LLC_REPLACEMENT}"
echo "Cores: ${NUM_CORE}"
BINARY_NAME="${CORE_UARCH}-${BRANCH}-${L1I_PREFETCHER}-${L1D_PREFETCHER}-${L2C_PREFETCHER}-${LLC_PREFETCHER}-${LLC_REPLACEMENT}-${NUM_CORE}core-${NUM_DRAM_CHANNELS}ch"
echo "Binary: bin/${BINARY_NAME}"
echo ""
mv bin/champsim bin/${BINARY_NAME}


# Restore to the default configuration
sed -i.bak 's/\<NUM_CPUS '${NUM_CORE}'\>/NUM_CPUS 1/g' inc/defs.h
sed -i.bak 's/\<DRAM_CHANNELS '${NUM_DRAM_CHANNELS}'\>/DRAM_CHANNELS 1/g' inc/defs.h
sed -i.bak 's/\<LOG2_DRAM_CHANNELS '${LOG_NUM_DRAM_CHANNELS}'\>/LOG2_DRAM_CHANNELS 0/g' inc/defs.h

cp inc/uarch/glc.h inc/defs.h
cp branch/bimodal.bpred branch/branch_predictor.cc
cp prefetcher/no.l1i_pref prefetcher/l1i_prefetcher.cc
cp prefetcher/no.l1d_pref prefetcher/l1d_prefetcher.cc
cp prefetcher/no.l2c_pref prefetcher/l2c_prefetcher.cc
cp prefetcher/no.llc_pref prefetcher/llc_prefetcher.cc
cp replacement/lru.llc_repl replacement/llc_replacement.cc
