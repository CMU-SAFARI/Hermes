#!/bin/bash

#####################################################################
#
# This is an automated script aimed to fix an unintentional
# trace downloading bug present in the v1.0.0.
#
# If you are Hermes <= v1.0.0, please execute this script
# to properly rename the downloaded trace names before the md5sum.
# As the trace names got mismatched only *within* the same workload
# (e.g., traces 429.mcf-184B and 429.mcf-22B got interchanged),
# NO REPRODUCED RESULTS WILL BE IMPACTED.
#
# If you are using Hermes > v1.0.0, please ignore this script.
#
#
# Author: Rahul Bera (write2bera@gmail.com)
#
######################################################################

# Fix 429.mcf 
mv 429.mcf-184B.champsimtrace.xz 429.mcf-22B.champsimtrace.xz.tmp
mv 429.mcf-217B.champsimtrace.xz 429.mcf-51B.champsimtrace.xz.tmp
mv 429.mcf-22B.champsimtrace.xz 429.mcf-184B.champsimtrace.xz.tmp
mv 429.mcf-51B.champsimtrace.xz 429.mcf-217B.champsimtrace.xz.tmp
mv 429.mcf-22B.champsimtrace.xz.tmp 429.mcf-22B.champsimtrace.xz
mv 429.mcf-51B.champsimtrace.xz.tmp 429.mcf-51B.champsimtrace.xz
mv 429.mcf-184B.champsimtrace.xz.tmp 429.mcf-184B.champsimtrace.xz
mv 429.mcf-217B.champsimtrace.xz.tmp 429.mcf-217B.champsimtrace.xz

# Fix 450.soplex
mv 450.soplex-247B.champsimtrace.xz 450.soplex-92B.champsimtrace.xz.tmp
mv 450.soplex-92B.champsimtrace.xz 450.soplex-247B.champsimtrace.xz.tmp
mv 450.soplex-92B.champsimtrace.xz.tmp 450.soplex-92B.champsimtrace.xz
mv 450.soplex-247B.champsimtrace.xz.tmp 450.soplex-247B.champsimtrace.xz

# Fix 602.gcc_s
mv 602.gcc_s-1850B.champsimtrace.xz 602.gcc_s-734B.champsimtrace.xz.tmp
mv 602.gcc_s-2226B.champsimtrace.xz 602.gcc_s-1850B.champsimtrace.xz.tmp
mv 602.gcc_s-734B.champsimtrace.xz 602.gcc_s-2226B.champsimtrace.xz.tmp
mv 602.gcc_s-734B.champsimtrace.xz.tmp 602.gcc_s-734B.champsimtrace.xz
mv 602.gcc_s-1850B.champsimtrace.xz.tmp 602.gcc_s-1850B.champsimtrace.xz
mv 602.gcc_s-2226B.champsimtrace.xz.tmp 602.gcc_s-2226B.champsimtrace.xz

# Fix 603.bwaves_s 
mv 603.bwaves_s-1740B.champsimtrace.xz 603.bwaves_s-891B.champsimtrace.xz.tmp
mv 603.bwaves_s-2609B.champsimtrace.xz 603.bwaves_s-1740B.champsimtrace.xz.tmp
mv 603.bwaves_s-891B.champsimtrace.xz 603.bwaves_s-2609B.champsimtrace.xz.tmp
mv 603.bwaves_s-891B.champsimtrace.xz.tmp 603.bwaves_s-891B.champsimtrace.xz
mv 603.bwaves_s-1740B.champsimtrace.xz.tmp 603.bwaves_s-1740B.champsimtrace.xz
mv 603.bwaves_s-2609B.champsimtrace.xz.tmp 603.bwaves_s-2609B.champsimtrace.xz

# Fix 605.mcf_s 
mv 605.mcf_s-1152B.champsimtrace.xz 605.mcf_s-472B.champsimtrace.xz.tmp
mv 605.mcf_s-1536B.champsimtrace.xz 605.mcf_s-484B.champsimtrace.xz.tmp
mv 605.mcf_s-472B.champsimtrace.xz 605.mcf_s-782B.champsimtrace.xz.tmp
mv 605.mcf_s-484B.champsimtrace.xz 605.mcf_s-994B.champsimtrace.xz.tmp
mv 605.mcf_s-782B.champsimtrace.xz 605.mcf_s-1152B.champsimtrace.xz.tmp
mv 605.mcf_s-994B.champsimtrace.xz 605.mcf_s-1536B.champsimtrace.xz.tmp
mv 605.mcf_s-472B.champsimtrace.xz.tmp 605.mcf_s-472B.champsimtrace.xz
mv 605.mcf_s-484B.champsimtrace.xz.tmp 605.mcf_s-484B.champsimtrace.xz
mv 605.mcf_s-782B.champsimtrace.xz.tmp 605.mcf_s-782B.champsimtrace.xz
mv 605.mcf_s-994B.champsimtrace.xz.tmp 605.mcf_s-994B.champsimtrace.xz
mv 605.mcf_s-1152B.champsimtrace.xz.tmp 605.mcf_s-1152B.champsimtrace.xz
mv 605.mcf_s-1536B.champsimtrace.xz.tmp 605.mcf_s-1536B.champsimtrace.xz

# Fix 654.roms_s 
mv 654.roms_s-1390B.champsimtrace.xz 654.roms_s-523B.champsimtrace.xz.tmp
mv 654.roms_s-523B.champsimtrace.xz 654.roms_s-1390B.champsimtrace.xz.tmp
mv 654.roms_s-523B.champsimtrace.xz.tmp 654.roms_s-523B.champsimtrace.xz
mv 654.roms_s-1390B.champsimtrace.xz.tmp 654.roms_s-1390B.champsimtrace.xz