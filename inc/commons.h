/******************************************************************
 * This file defines all simulator parameters that are
 * invariant to the underlying simulated microarchitecture
 ******************************************************************/

#ifndef CHAMPSIM_COMMONS_H
#define CHAMPSIM_COMMONS_H

/*********************
 * Basic definitions
 *********************/
// instruction format
#define NUM_INSTR_DESTINATIONS_SPARC 4
#define NUM_INSTR_DESTINATIONS 2
#define NUM_INSTR_SOURCES 4

// CACHE BASICS
#define BLOCK_SIZE 64
#define LOG2_BLOCK_SIZE 6
#define MAX_READ_PER_CYCLE 8
#define MAX_FILL_PER_CYCLE 1

// Page basics
#define PAGE_SIZE 4096
#define LOG2_PAGE_SIZE 12

#define INFLIGHT 1
#define COMPLETED 2

#define FILL_L1     1
#define FILL_L2     2
#define FILL_LLC    4
#define FILL_DRC    8
#define FILL_DDRP   9
#define FILL_DRAM   16

// CACHE ACCESS TYPE
#define LOAD      0
#define RFO       1
#define PREFETCH  2
#define WRITEBACK 3
#define NUM_TYPES 4

// ROB PARTITION TYPES
#define FRONTAL             0
#define NONE                1
#define DORSAL              2
#define NUM_PARTITION_TYPES 3

// MODULE TYPE
#define IS_ITLB 0
#define IS_DTLB 1
#define IS_STLB 2
#define IS_L1I  3
#define IS_L1D  4
#define IS_L2C  5
#define IS_LLC  6
#define IS_DRAM 7
#define NUM_MODULE_TYPES 8

// QUEUE TYPES
#define IS_RQ           0
#define IS_WQ           1
#define IS_PQ           2
#define IS_MSHR         3
#define IS_PROCESSED    4
#define NUM_QUEUE_TYPES 5

// special registers that help us identify branches
#define REG_STACK_POINTER 6
#define REG_FLAGS 25
#define REG_INSTRUCTION_POINTER 26

// branch types
#define NOT_BRANCH           0
#define BRANCH_DIRECT_JUMP   1
#define BRANCH_INDIRECT      2
#define BRANCH_CONDITIONAL   3
#define BRANCH_DIRECT_CALL   4
#define BRANCH_INDIRECT_CALL 5
#define BRANCH_RETURN        6
#define BRANCH_OTHER         7

// dependent instruction types
#define DEP_INSTR_BRANCH_MISPRED    0
#define DEP_INSTR_BRANCH_CORRECT    1
#define DEP_INSTR_LOAD              2
#define DEP_INSTR_TYPES             3

// some more parameters for stat collection
#define DRAM_BW_LEVELS 4
#define CACHE_ACC_LEVELS 10

#endif /* CHAMPSIM_COMMONS_H */

