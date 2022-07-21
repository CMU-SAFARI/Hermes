/**************************************************************************************
 * This file defines a monitor that dynamically manages DDRP 
 * in presence of a traditional data prefetcher (e.g., Pythia).
 * The key idea is to monitor total cycles spent in commiting
 * N instructions in four following configurations:
 *    4) DDRP: ON  & data prefetcher: ON
 *    2) DDRP: ON  & data prefetcher: OFF
 *    1) DDRP: OFF & data prefetcher: ON
 *    3) DDRP: OFF & data prefetcher: OFF
 * The monitor exploits the configuration for executing the next M instructions
 * and then again switch back to exploration mode.
 *
 * Note that, the current implementation is not very extensible
 * in a sense that it completely disables DDRP or data prefetcher
 * Some data prefetchers, like Pythia, might have tunable parameters,
 * like reward for incorrect prefetching, to enforce accurate prefetching
 * without completely turning off the prefetcher entirely. 
 * 
 * Author: Rahul Bera (write2bera@gmail.com)
 * SAFARI Research Group, ETH Zürich
 **************************************************************************************/

#ifndef DDRP_MONITOR_H
#define DDRP_MONITOR_H

#include <cstring>
#include "util.h"
using namespace std;

typedef enum
{
    DDRP_ON_DP_ON = 0,
    DDRP_ON_DP_OFF,
    DDRP_OFF_DP_ON,
    DDRP_OFF_DP_OFF,

    NumDDRPMonitorConfigs
} ddrp_monitor_config_t;

extern string ddrp_monitor_config_string[NumDDRPMonitorConfigs];

typedef enum
{
    EXPLORE = 0,
    EXPLOIT,

    NumDDRPMonitorPhases,
} ddrp_monitor_phase_t;

class DDRPMonitor
{
    public:
        uint32_t cpu;
        ddrp_monitor_config_t config;
        ddrp_monitor_phase_t phase;
        uint64_t cycles[NumDDRPMonitorConfigs];
        uint64_t instr_count;
        uint64_t cycle_stamp;
        
        bool disable_ddrp;
        bool disable_data_prefetcher;

        // for hysterisis
        ddrp_monitor_config_t prev_winner_config;
        Counter prev_winner_config_confidence;

        // remember the original config of Pythia
        // int32_t orig_scooby_reward_incorrect, orig_scooby_reward_none;

        // stats
        struct
        {
            struct
            {
                uint64_t called;
                uint64_t exploit_to_explore;
                uint64_t in_exploit;
                uint64_t explore_to_exploit;
                uint64_t explore_another_config;
                uint64_t in_explore;
            } monitor;

            struct
            {
                uint64_t called;
                uint64_t histogram[NumDDRPMonitorConfigs];
            } winner_config;

            struct
            {
                uint64_t cfg_match[NumDDRPMonitorConfigs];
                uint64_t cfg_mismatch_change[NumDDRPMonitorConfigs][NumDDRPMonitorConfigs];
                uint64_t cfg_mismatch_unchanged[NumDDRPMonitorConfigs][NumDDRPMonitorConfigs];
            } hysterisis;

        } stats;

    public:
        DDRPMonitor(uint32_t cpu);
        ~DDRPMonitor();
        void print_config(),
             dump_stats(),
             reset_stats(),
             reset_cycles(),
             set_params();
             
        ddrp_monitor_config_t get_winner_config();
        void monitor_instr(uint64_t core_cycle);
};


#endif /* DDRP_MONITOR_H */

