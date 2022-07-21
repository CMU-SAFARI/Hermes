#!/bin/bash

echo "Building gen_fwd_reuse.cc ..."
g++ --std=c++11 -O2 gen_fwd_reuse.cc -o gen_fwd_reuse -lz

echo "Building optcache_driver.cc ..."
g++ --std=c++11 optcache_driver.cc -O3 -o optcache_driver -lz
