/******************************************************************
 * This file defines all core+uncore configutation parameters
 * The core parameters are closely modeled after Apple A14 Firestorm
 *
 * https://www.anandtech.com/show/16226/apple-silicon-m1-a14-deep-dive/2
 *
 ******************************************************************/

#ifndef CHAMPSIM_DEFS_H
#define CHAMPSIM_DEFS_H

#include "commons.h"

/*********************
 * Simulation params *
 *********************/
#define NUM_CPUS 1
#define CPU_FREQ 4000 // Firestorm runs on 3 GHz

/*********************
 * Fetch/decode stage *
 *********************/
#define FETCH_WIDTH 8
#define DECODE_WIDTH 8

// Branch predictor
#define BRANCH_MISPREDICT_PENALTY 17

// INSTRUCTION TLB
#define ITLB_SET 64
#define ITLB_WAY 4
#define ITLB_RQ_SIZE 16
#define ITLB_WQ_SIZE 16
#define ITLB_PQ_SIZE 0
#define ITLB_MSHR_SIZE 8
#define ITLB_LATENCY 1

// L1 INSTRUCTION CACHE
#define L1I_SET 256 // 192KB LI-I
#define L1I_WAY 12
#define L1I_RQ_SIZE 64
#define L1I_WQ_SIZE 64 
#define L1I_PQ_SIZE 32
#define L1I_MSHR_SIZE 8
#define L1I_LATENCY 3

/******************************
 * Reordering/execution stage *
 ******************************/
#define ROB_SIZE 632
// SCHEDULER_SIZE needs a better implementation
// ideally it should model reservation station size
// but it is modelled in a bad way, thus setting this to ROB_SIZE
// Golden Cove's reservation station size is ~192 entries
#define SCHEDULER_SIZE ROB_SIZE
#define STA_SIZE (ROB_SIZE*NUM_INSTR_DESTINATIONS_SPARC)
#define EXEC_WIDTH 8

/************************
 * Memory access stage *
 ************************/
#define LQ_SIZE 148
#define SQ_SIZE 106
#define LQ_WIDTH 2
#define SQ_WIDTH 2

// DATA TLB
#define DTLB_SET 16
#define DTLB_WAY 8
#define DTLB_RQ_SIZE 16
#define DTLB_WQ_SIZE 16
#define DTLB_PQ_SIZE 0
#define DTLB_MSHR_SIZE 8
#define DTLB_LATENCY 1

// SECOND LEVEL TLB
#define STLB_SET 256
#define STLB_WAY 12
#define STLB_RQ_SIZE 32
#define STLB_WQ_SIZE 32
#define STLB_PQ_SIZE 0
#define STLB_MSHR_SIZE 16
#define STLB_LATENCY 8

// L1 DATA CACHE
#define L1D_SET 256 // 128KB L1-D
#define L1D_WAY 8
#define L1D_RQ_SIZE 64
#define L1D_WQ_SIZE 64
#define L1D_PQ_SIZE 8
#define L1D_MSHR_SIZE 16
#define L1D_LATENCY 3

// L2 CACHE
#define L2C_SET 4096 // A14 Firestorm has 8MB L2 shared across two big cores
#define L2C_WAY 16
#define L2C_RQ_SIZE 48
#define L2C_WQ_SIZE 48
#define L2C_PQ_SIZE 16
#define L2C_MSHR_SIZE 48
#define L2C_LATENCY 16

// LAST LEVEL CACHE
#define LLC_SET NUM_CPUS*8192 // A14 has 16MB shared across all on-chip IPs
#define LLC_WAY 16
#define LLC_RQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_WQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_PQ_SIZE NUM_CPUS*32
#define LLC_MSHR_SIZE NUM_CPUS*64
#define LLC_LATENCY 24

/****************
 * Retire stage *
 ****************/
#define RETIRE_WIDTH 8

/****************
 * Main memory *
 ****************/
#define DRAM_CHANNELS 1      // default: assuming one DIMM per one channel 4GB * 1 => 4GB off-chip memory
#define LOG2_DRAM_CHANNELS 0
#define DRAM_RANKS 1         // 512MB * 8 ranks => 4GB per DIMM
#define LOG2_DRAM_RANKS 0
#define DRAM_BANKS 8         // 64MB * 8 banks => 512MB per rank
#define LOG2_DRAM_BANKS 3
#define DRAM_ROWS 65536      // 2KB * 32K rows => 64MB per bank
#define LOG2_DRAM_ROWS 16
#define DRAM_COLUMNS 128      // 64B * 32 column chunks (Assuming 1B DRAM cell * 8 chips * 8 transactions = 64B size of column chunks) => 2KB per row
#define LOG2_DRAM_COLUMNS 7
#define DRAM_ROW_SIZE (BLOCK_SIZE*DRAM_COLUMNS/1024)
#define DRAM_SIZE (DRAM_CHANNELS*DRAM_RANKS*DRAM_BANKS*DRAM_ROWS*DRAM_ROW_SIZE/1024) 
#define DRAM_PAGES ((DRAM_SIZE<<10)>>2)

// DRAM configuration
#define DRAM_CHANNEL_WIDTH 8 // 8B
#define DRAM_WQ_SIZE 64
#define DRAM_RQ_SIZE 64

#define tRP_DRAM_NANOSECONDS  12.5
#define tRCD_DRAM_NANOSECONDS 12.5
#define tCAS_DRAM_NANOSECONDS 12.5

// the data bus must wait this amount of time when switching between reads and writes, and vice versa
#define DRAM_DBUS_TURN_AROUND_TIME ((15*CPU_FREQ)/2000) // 7.5 ns 

// these values control when to send out a burst of writes
#define DRAM_WRITE_HIGH_WM    ((DRAM_WQ_SIZE*7)>>3) // 7/8th
#define DRAM_WRITE_LOW_WM     ((DRAM_WQ_SIZE*3)>>2) // 6/8th
#define MIN_DRAM_WRITES_PER_SWITCH (DRAM_WQ_SIZE*1/4)

#endif /* CHAMPSIM_DEFS_H */

