#include <iostream>
#include <string.h>
#include <assert.h>
#include <vector>
#include "ddrp_monitor.h"
#include "champsim.h"

#if 0
#define MYLOG(cond, ...)                                        \
    if (cond)                                                   \
    {                                                           \
        fprintf(stdout, "[%25s@%3u] ", __FUNCTION__, __LINE__); \
        fprintf(stdout, __VA_ARGS__);                           \
        fprintf(stdout, "\n");                                  \
        fflush(stdout);                                         \
    }
#else
#define MYLOG(cond, ...) \
    {                    \
    }
#endif

namespace knob
{
    extern uint32_t ddrp_monitor_exploit_epoch;
    extern uint32_t ddrp_monitor_explore_epoch;
    // extern int32_t  ddrp_monitor_scooby_reward_incorrect;
    // extern int32_t  ddrp_monitor_scooby_reward_none;
    extern bool enable_ddrp;
    // extern int32_t scooby_reward_incorrect;
    // extern int32_t scooby_reward_none;
    // extern vector<string> l2c_prefetcher_types;
    extern bool ddrp_monitor_enable_hysterisis;
}

string ddrp_monitor_config_string[] = {
    "DDRP_ON_DP_ON",
    "DDRP_ON_DP_OFF",
    "DDRP_OFF_DP_ON",
    "DDRP_OFF_DP_OFF"
};

DDRPMonitor::DDRPMonitor(uint32_t _cpu) : cpu(_cpu)
{
    // currently DDRP monitor only supports Pythia at L2
    // for(uint32_t index = 0; index < knob::l2c_prefetcher_types.size(); ++index)
    // {
    //     assert(knob::l2c_prefetcher_types[index] == "none" || knob::l2c_prefetcher_types[index] == "scooby");
    // }

    config = ddrp_monitor_config_t::DDRP_ON_DP_ON;
    phase = ddrp_monitor_phase_t::EXPLORE;
    reset_cycles();
    instr_count = 0;
    cycle_stamp = 0;
    
    disable_ddrp = false;
    disable_data_prefetcher = false;

    if(knob::ddrp_monitor_enable_hysterisis)
    {
        prev_winner_config = DDRP_OFF_DP_OFF;
        prev_winner_config_confidence.init(3, 0);
    }

    // orig_scooby_reward_incorrect = knob::scooby_reward_incorrect;
    // orig_scooby_reward_none = knob::scooby_reward_none;
    reset_stats();

    cout << "Adding DDRP monitor"
        //  << " orig_scooby_reward_incorrect: " << orig_scooby_reward_incorrect 
        //  << " orig_scooby_reward_none: " << orig_scooby_reward_none 
         << endl;
}

DDRPMonitor::~DDRPMonitor()
{

}

void DDRPMonitor::print_config()
{
    cout << "ddrp_monitor_exploit_epoch " << knob::ddrp_monitor_exploit_epoch << endl
         << "ddrp_monitor_explore_epoch " << knob::ddrp_monitor_explore_epoch << endl
        //  << "ddrp_monitor_scooby_reward_incorrect " << knob::ddrp_monitor_scooby_reward_incorrect << endl
        //  << "ddrp_monitor_scooby_reward_none " << knob::ddrp_monitor_scooby_reward_none << endl
         << "ddrp_monitor_enable_hysterisis " << knob::ddrp_monitor_enable_hysterisis << endl
         << endl;
}

void DDRPMonitor::reset_cycles()
{
    for(uint32_t index = 0; index < NumDDRPMonitorConfigs; ++index)
    {
        cycles[index] = 0;
    }
}

void DDRPMonitor::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

void DDRPMonitor::set_params()
{
    if(config == ddrp_monitor_config_t::DDRP_ON_DP_ON)
    {
        disable_ddrp = false;
        disable_data_prefetcher = false;
        // knob::scooby_reward_incorrect = orig_scooby_reward_incorrect;
        // knob::scooby_reward_none = orig_scooby_reward_none;
    }
    else if(config == ddrp_monitor_config_t::DDRP_ON_DP_OFF)
    {
        disable_ddrp = false;
        disable_data_prefetcher = true;
        // knob::scooby_reward_incorrect = knob::ddrp_monitor_scooby_reward_incorrect;
        // knob::scooby_reward_none = knob::ddrp_monitor_scooby_reward_none;
    }
    else if(config == ddrp_monitor_config_t::DDRP_OFF_DP_ON)
    {
        disable_ddrp = true;
        disable_data_prefetcher = false;
        // knob::scooby_reward_incorrect = orig_scooby_reward_incorrect;
        // knob::scooby_reward_none = orig_scooby_reward_none;
    }
    else if(config == ddrp_monitor_config_t::DDRP_OFF_DP_OFF)
    {
        disable_ddrp = true;
        disable_data_prefetcher = true;
        // knob::scooby_reward_incorrect = knob::ddrp_monitor_scooby_reward_incorrect;
        // knob::scooby_reward_none = knob::ddrp_monitor_scooby_reward_none;
    }
    else
    {
        // should not come here
        assert(false);
    }
}

void DDRPMonitor::monitor_instr(uint64_t curr_core_cycle)
{
    stats.monitor.called++;
    instr_count++;

    // state machine design
    if(phase == ddrp_monitor_phase_t::EXPLOIT)
    {
        if(instr_count >= knob::ddrp_monitor_exploit_epoch)
        {
            // transition to explore mode
            phase = ddrp_monitor_phase_t::EXPLORE;
            config = ddrp_monitor_config_t::DDRP_ON_DP_ON;
            instr_count = 0;
            cycle_stamp = curr_core_cycle;
            reset_cycles();
            set_params();
            MYLOG(warmup_complete[cpu], "exploit -> explore");
            stats.monitor.exploit_to_explore++;
        }
        else
        {
            // do nothing
            stats.monitor.in_exploit++;
        }
    }
    else if(phase == ddrp_monitor_phase_t::EXPLORE)
    {
        if(instr_count >= knob::ddrp_monitor_explore_epoch)
        {
            // record cycle count
            cycles[config] = (curr_core_cycle - cycle_stamp);

            // transition to exploit if this is the last config in exploration
            if(config == ddrp_monitor_config_t::DDRP_OFF_DP_OFF)
            {
                MYLOG(warmup_complete[cpu], "explore -> exploit cycles taken in last explore config: %ld", cycles[config]);
                phase = ddrp_monitor_phase_t::EXPLOIT;
                config = get_winner_config();
                reset_cycles();
                set_params();
                MYLOG(warmup_complete[cpu], "winner config: %s", ddrp_monitor_config_string[config].c_str());
                stats.monitor.explore_to_exploit++;
            }
            // otherwise transition to next config
            else
            {
                phase = ddrp_monitor_phase_t::EXPLORE;
                MYLOG(warmup_complete[cpu], "explore config transition:: old_cfg: %s", ddrp_monitor_config_string[config].c_str());
                config = (ddrp_monitor_config_t)((uint32_t)config + 1);
                MYLOG(warmup_complete[cpu], "new_cfg: %s", ddrp_monitor_config_string[config].c_str());
                set_params();
                stats.monitor.explore_another_config++;
            }

            instr_count = 0;
            cycle_stamp = curr_core_cycle;
        }
        else
        {
            // do nothing
            stats.monitor.in_explore++;
        }
    }
    else
    {
        // should not come here
        assert(false);
    }
}

ddrp_monitor_config_t DDRPMonitor::get_winner_config()
{
    stats.winner_config.called++;
    uint64_t min_cycle = UINT64_MAX;
    uint32_t min_cfg = 0;
    for(uint32_t index = 0; index < NumDDRPMonitorConfigs; ++index)
    {
        assert(cycles[index]);
        if(cycles[index] < min_cycle)
        {
            min_cycle = cycles[index];
            min_cfg = index;
        }
    }

    if(knob::ddrp_monitor_enable_hysterisis)
    {
        if(min_cfg == prev_winner_config)
        {
            prev_winner_config_confidence.incr();
            stats.hysterisis.cfg_match[min_cfg]++;
        }
        else
        {
            assert(min_cfg != prev_winner_config);
            if(prev_winner_config_confidence.val() > 0)
            {
                stats.hysterisis.cfg_mismatch_unchanged[min_cfg][prev_winner_config]++;
                min_cfg = prev_winner_config;
                prev_winner_config_confidence.decr();
            }
            else
            {
                stats.hysterisis.cfg_mismatch_change[min_cfg][prev_winner_config]++;
                prev_winner_config = (ddrp_monitor_config_t)min_cfg;
                prev_winner_config_confidence.reset();
            }
        }
    }

    stats.winner_config.histogram[min_cfg]++;
    MYLOG(warmup_complete[cpu], "cycle_DDRP_ON_DP_ON: %ld cycle_DDRP_ON_DP_OFF: %ld cycle_DDRP_OFF_DP_ON: %ld cycle_DDRP_OFF_DP_OFF: %ld", 
                                cycles[DDRP_ON_DP_ON], cycles[DDRP_ON_DP_OFF], cycles[DDRP_OFF_DP_ON], cycles[DDRP_OFF_DP_OFF]);
    return (ddrp_monitor_config_t)min_cfg;
}

void DDRPMonitor::dump_stats()
{
    cout << "monitor_called " << stats.monitor.called << endl
         << "monitor_exploit_to_explore " << stats.monitor.exploit_to_explore << endl
         << "monitor_in_exploit " << stats.monitor.in_exploit << endl
         << "monitor_explore_to_exploit " << stats.monitor.explore_to_exploit << endl
         << "monitor_explore_another_config " << stats.monitor.explore_another_config << endl
         << "monitor_in_explore " << stats.monitor.in_explore << endl
         << endl
         << "winner_config_called " << stats.winner_config.called << endl
         << "winner_config_DDRP_ON_DP_ON " << stats.winner_config.histogram[DDRP_ON_DP_ON] << endl
         << "winner_config_DDRP_ON_DP_OFF " << stats.winner_config.histogram[DDRP_ON_DP_OFF] << endl
         << "winner_config_DDRP_OFF_DP_ON " << stats.winner_config.histogram[DDRP_OFF_DP_ON] << endl
         << "winner_config_DDRP_OFF_DP_OFF " << stats.winner_config.histogram[DDRP_OFF_DP_OFF] << endl
         << endl;

    if(knob::ddrp_monitor_enable_hysterisis)
    {
        for(uint32_t cfg = 0; cfg < NumDDRPMonitorConfigs; ++cfg)
        {
            cout << "monitor_hysterisis_cfg_match_" << cfg << " " << stats.hysterisis.cfg_match[cfg] << endl;
            for(uint32_t cfg2 = 0; cfg2 < NumDDRPMonitorConfigs; ++cfg2)
            {
                cout << "monitor_hysterisis_cfg_mismatch_" << cfg << "_change_from_" << cfg2 << " " << stats.hysterisis.cfg_mismatch_change[cfg][cfg2] << endl
                     << "monitor_hysterisis_cfg_mismatch_" << cfg << "_unchange_from_" << cfg2 << " " << stats.hysterisis.cfg_mismatch_unchanged[cfg][cfg2] << endl
                     << endl;
            }
        }
    }
}
