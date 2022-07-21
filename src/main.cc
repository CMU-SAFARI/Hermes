#define _BSD_SOURCE

#include <getopt.h>
#include <string.h>
#include "ooo_cpu.h"
#include "uncore.h"
#include "knobs.h"
#include "util.h"
#include <fstream>
#include <algorithm>
#include <numeric>

#define FIXED_FLOAT(x) std::fixed << std::setprecision(5) << (x)
// #define PRINT_AUX_STATS 1

#if 0
#   define LOCKED(...) {fflush(stdout); __VA_ARGS__; fflush(stdout);}
#   define LOGID() fprintf(stdout, "[%25s@%3u] ", \
                            __FUNCTION__, __LINE__ \
                            );
#   define MYLOG(...) LOCKED(LOGID(); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#   define MYLOG(...) {}
#endif

namespace knob
{
    extern uint64_t warmup_instructions;
    extern uint64_t simulation_instructions;
    extern bool     cloudsuite;
    extern bool     low_bandwidth;
    extern bool     measure_ipc;
    extern uint32_t measure_ipc_epoch;
    extern uint32_t dram_io_freq;
    extern bool     measure_dram_bw;
    extern uint64_t measure_dram_bw_epoch;
    extern bool     measure_cache_acc;
    extern uint64_t measure_cache_acc_epoch;
    extern bool     l1d_perfect;
    extern bool     l2c_perfect;
    extern bool     llc_perfect;
    extern bool     l1d_semi_perfect;
    extern bool     l2c_semi_perfect;
    extern bool     llc_semi_perfect;
    extern uint32_t semi_perfect_cache_page_buffer_size;
    extern bool     enable_offchip_tracing;
    extern string   offchip_trace_filename;
    extern bool     l2c_dump_access_trace;
    extern bool     llc_dump_access_trace;
    extern string   l2c_access_trace_filename;
	extern string   llc_access_trace_filename;
    extern uint32_t l2c_dump_access_trace_type;
    extern uint32_t llc_dump_access_trace_type;
    extern bool     track_load_hit_dependency_in_cache;
    extern uint32_t load_hit_dependency_max_level;
    extern bool     llc_pseudo_perfect_enable;
    extern float    llc_pseudo_perfect_prob;
    extern bool     llc_pseudo_perfect_enable_frontal;
    extern bool     llc_pseudo_perfect_enable_dorsal;
    extern bool     l2c_pseudo_perfect_enable;
    extern float    l2c_pseudo_perfect_prob;
    extern bool     l2c_pseudo_perfect_enable_frontal;
    extern bool     l2c_pseudo_perfect_enable_dorsal;
	extern uint32_t num_rob_partitions;
	extern vector<int32_t> rob_partition_size;
	extern vector<int32_t> rob_partition_boundaries;
    extern vector<int32_t> rob_frontal_partition_ids;
    extern vector<int32_t> rob_dorsal_partition_ids;
    extern bool     enable_pseudo_direct_dram_prefetch;
    extern bool     enable_pseudo_direct_dram_prefetch_on_prefetch;
    extern uint32_t pseudo_direct_dram_prefetch_rob_part_type;
    extern bool     enable_ddrp;
    extern uint32_t ddrp_req_latency;
    extern bool     offchip_pred_mark_merged_load;
    extern bool     dram_cntlr_enable_ddrp_buffer;
    extern uint32_t dram_cntlr_ddrp_buffer_sets;
    extern uint32_t dram_cntlr_ddrp_buffer_assoc;
    extern uint32_t dram_cntlr_ddrp_buffer_hash_type;
    extern bool     enable_ddrp_monitor;
}

uint8_t warmup_complete[NUM_CPUS], 
        simulation_complete[NUM_CPUS], 
        all_warmup_complete = 0, 
        all_simulation_complete = 0,
        MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS;
uint64_t champsim_seed;

time_t start_time;

// PAGE TABLE
uint32_t PAGE_TABLE_LATENCY = 0, SWAP_LATENCY = 0;
queue <uint64_t > page_queue;
map <uint64_t, uint64_t> page_table, inverse_table, recent_page, unique_cl[NUM_CPUS];
uint64_t previous_ppage, num_adjacent_page, num_cl[NUM_CPUS], allocated_pages, num_page[NUM_CPUS], minor_fault[NUM_CPUS], major_fault[NUM_CPUS];

string is_data_names[2] = {"instruction", "data"};
string type_names[NUM_TYPES] = {"load", "RFO", "prefetch", "writeback"};

void print_knobs()
{
    cout << "warmup_instructions " << knob::warmup_instructions << endl
        << "simulation_instructions " << knob::simulation_instructions << endl
        << "champsim_seed " << champsim_seed << endl
        << "low_bandwidth " << knob::low_bandwidth << endl
        // << "scramble_loads " << knob_scramble_loads << endl
        << "cloudsuite " << knob::cloudsuite << endl
        << "measure_ipc " << knob::measure_ipc << endl
        << "measure_ipc_epoch " << knob::measure_ipc_epoch << endl
        << "measure_dram_bw " << knob::measure_dram_bw << endl
        << "measure_dram_bw_epoch " << knob::measure_dram_bw_epoch << endl
        << "measure_cache_acc " << knob::measure_cache_acc << endl
        << "measure_cache_acc_epoch " << knob::measure_cache_acc_epoch << endl
        << "l1d_perfect " << knob::l1d_perfect << endl
        << "l2c_perfect " << knob::l2c_perfect << endl
        << "llc_perfect " << knob::llc_perfect << endl
        << "l1d_semi_perfect " << knob::l1d_semi_perfect << endl
        << "l2c_semi_perfect " << knob::l2c_semi_perfect << endl
        << "llc_semi_perfect " << knob::llc_semi_perfect << endl
        << "semi_perfect_cache_page_buffer_size " << knob::semi_perfect_cache_page_buffer_size << endl
        << "enable_offchip_tracing " << knob::enable_offchip_tracing << endl
        << "offchip_trace_filename " << knob::offchip_trace_filename << endl
        << "l2c_dump_access_trace " << knob::l2c_dump_access_trace << endl
        << "llc_dump_access_trace " << knob::llc_dump_access_trace << endl
        << "l2c_access_trace_filename " << knob::l2c_access_trace_filename << endl
        << "llc_access_trace_filename " << knob::llc_access_trace_filename << endl
        << "l2c_dump_access_trace_type " << knob::l2c_dump_access_trace_type << endl
        << "llc_dump_access_trace_type " << knob::llc_dump_access_trace_type << endl
        << "track_load_hit_dependency_in_cache " << knob::track_load_hit_dependency_in_cache << endl
        << "load_hit_dependency_max_level " << knob::load_hit_dependency_max_level << endl
        << "llc_pseudo_perfect_enable " << knob::llc_pseudo_perfect_enable << endl
        << "llc_pseudo_perfect_prob " << knob::llc_pseudo_perfect_prob << endl
        << "llc_pseudo_perfect_enable_frontal " << knob::llc_pseudo_perfect_enable_frontal << endl
        << "llc_pseudo_perfect_enable_dorsal " << knob::llc_pseudo_perfect_enable_dorsal << endl
        << "l2c_pseudo_perfect_enable " << knob::l2c_pseudo_perfect_enable << endl
        << "l2c_pseudo_perfect_prob " << knob::l2c_pseudo_perfect_prob << endl
        << "l2c_pseudo_perfect_enable_frontal " << knob::l2c_pseudo_perfect_enable_frontal << endl
        << "l2c_pseudo_perfect_enable_dorsal " << knob::l2c_pseudo_perfect_enable_dorsal << endl
        << "num_rob_partitions " << knob::num_rob_partitions << endl
        << "rob_partition_size " << array_to_string(knob::rob_partition_size) << endl
        << "rob_partition_boundaries " << array_to_string(knob::rob_partition_boundaries) << endl
        << "rob_frontal_partition_ids " << array_to_string(knob::rob_frontal_partition_ids) << endl
        << "rob_dorsal_partition_ids " << array_to_string(knob::rob_dorsal_partition_ids) << endl
        << "enable_pseudo_direct_dram_prefetch " << knob::enable_pseudo_direct_dram_prefetch << endl
        << "enable_pseudo_direct_dram_prefetch_on_prefetch " << knob::enable_pseudo_direct_dram_prefetch_on_prefetch << endl
        << "pseudo_direct_dram_prefetch_rob_part_type " << knob::pseudo_direct_dram_prefetch_rob_part_type << endl
        << "offchip_pred_mark_merged_load " << knob::offchip_pred_mark_merged_load << endl
        << "enable_ddrp " << knob::enable_ddrp << endl
        << "ddrp_req_latency " << knob::ddrp_req_latency << endl
        << "dram_cntlr_enable_ddrp_buffer " << knob::dram_cntlr_enable_ddrp_buffer << endl
        << "dram_cntlr_ddrp_buffer_sets " << knob::dram_cntlr_ddrp_buffer_sets << endl
        << "dram_cntlr_ddrp_buffer_assoc " << knob::dram_cntlr_ddrp_buffer_assoc << endl
        << "dram_cntlr_ddrp_buffer_hash_type " << knob::dram_cntlr_ddrp_buffer_hash_type << endl
        << "enable_ddrp_monitor " << knob::enable_ddrp_monitor << endl
<< endl;

    cout << "num_cpus " << NUM_CPUS << endl
        << "cpu_freq " << CPU_FREQ << endl
        << "dram_io_freq " << knob::dram_io_freq << endl
        << "page_size " << PAGE_SIZE << endl
        << "block_size " << BLOCK_SIZE << endl
        << "max_read_per_cycle " << MAX_READ_PER_CYCLE << endl
        << "max_fill_per_cycle " << MAX_FILL_PER_CYCLE << endl
        << "dram_channels " << DRAM_CHANNELS << endl
	<< "log2_dram_channels " << LOG2_DRAM_CHANNELS << endl
        << "dram_ranks " << DRAM_RANKS << endl
        << "dram_banks " << DRAM_BANKS << endl
        << "dram_rows " << DRAM_ROWS << endl
        << "dram_columns " << DRAM_COLUMNS << endl
        << "dram_row_size " << DRAM_ROW_SIZE << endl
        << "dram_size " << DRAM_SIZE << endl
        << "dram_pages " << DRAM_PAGES << endl
        << endl;
    
    print_core_config();
    print_cache_config();
    print_dram_config();

    ooo_cpu[0].l1i_prefetcher_print_config();
    ooo_cpu[0].L1D.l1d_prefetcher_print_config();
    ooo_cpu[0].L2C.l2c_prefetcher_print_config();
    uncore.LLC.llc_prefetcher_print_config();
    uncore.LLC.llc_replacement_print_config();
    ooo_cpu[0].print_config_offchip_predictor();
    ooo_cpu[0].print_config_ddrp_monitor();
    
    cout << endl;
}

void record_roi_stats(uint32_t cpu, CACHE *cache)
{
    for (uint32_t i=0; i<NUM_TYPES; i++) {
        cache->roi_access[cpu][i] = cache->sim_access[cpu][i];
        cache->roi_hit[cpu][i] = cache->sim_hit[cpu][i];
        cache->roi_miss[cpu][i] = cache->sim_miss[cpu][i];
    }
}

void print_core_roi_stats(uint32_t cpu)
{
    // load-induced bubble stats
    cout << "Core_" << cpu << "_bubble_called " << ooo_cpu[cpu].stats.bubble.called << endl
         << "Core_" << cpu << "_bubble_rob_non_head " << ooo_cpu[cpu].stats.bubble.rob_non_head << endl
         << "Core_" << cpu << "_bubble_rob_head " << ooo_cpu[cpu].stats.bubble.rob_head << endl
         << "Core_" << cpu << "_bubble_went_offchip " << ooo_cpu[cpu].stats.bubble.went_offchip << endl
         << "Core_" << cpu << "_bubble_went_offchip_rob_head " << ooo_cpu[cpu].stats.bubble.went_offchip_rob_head << endl
         << "Core_" << cpu << "_bubble_went_offchip_rob_non_head " << ooo_cpu[cpu].stats.bubble.went_offchip_rob_non_head << endl
         << endl;
    
    for(uint32_t index = 0; index < knob::num_rob_partitions; ++index)
    {
        cout << "Core_" << cpu << "_bubble_rob_part_" << index << "_max " << ooo_cpu[cpu].bubble_max[index] << endl
             << "Core_" << cpu << "_bubble_rob_part_" << index << "_min " << ooo_cpu[cpu].bubble_min[index] << endl
             << "Core_" << cpu << "_bubble_rob_part_" << index << "_cnt " << ooo_cpu[cpu].bubble_cnt[index] << endl
             << "Core_" << cpu << "_bubble_rob_part_" << index << "_tot " << ooo_cpu[cpu].bubble_tot[index] << endl
             << "Core_" << cpu << "_bubble_rob_part_" << index << "_avg " << (float)ooo_cpu[cpu].bubble_tot[index]/ooo_cpu[cpu].bubble_cnt[index] << endl;
    }
    cout << endl;

    // loads per ip stats
    std::vector<std::pair<uint64_t, load_per_ip_info_t>> pairs;
    for(auto it = ooo_cpu[cpu].load_per_ip_stats.begin(); it != ooo_cpu[cpu].load_per_ip_stats.end(); ++it)
        pairs.push_back(*it);
    std::sort(pairs.begin(), pairs.end(), 
        [](const std::pair<uint64_t, load_per_ip_info_t> &p1, const std::pair<uint64_t, load_per_ip_info_t> &p2)
        {
            return p1.second.loads_went_offchip > p2.second.loads_went_offchip;
        });
    
    uint64_t total_loads = 0, total_loads_went_offchip = 0, accum = 0;
    for(auto it = pairs.begin(); it != pairs.end(); ++it)
    {
        total_loads += it->second.total_loads;
        total_loads_went_offchip += it->second.loads_went_offchip;
    }
        
    cout << "Core_" << cpu << "_total_loads " << total_loads << endl
         << "Core_" << cpu << "_total_loads_went_offchip " << total_loads_went_offchip << endl
         << "Core_" << cpu << "_total_frontal_loads " << ooo_cpu[cpu].load_per_rob_part_stats[FRONTAL].total_loads << endl
         << "Core_" << cpu << "_total_frontal_loads_went_offchip " << ooo_cpu[cpu].load_per_rob_part_stats[FRONTAL].loads_went_offchip << endl
         << "Core_" << cpu << "_total_dorsal_loads " << ooo_cpu[cpu].load_per_rob_part_stats[DORSAL].total_loads << endl
         << "Core_" << cpu << "_total_dorsal_loads_went_offchip " << ooo_cpu[cpu].load_per_rob_part_stats[DORSAL].loads_went_offchip << endl
         << "Core_" << cpu << "_total_none_loads " << ooo_cpu[cpu].load_per_rob_part_stats[NONE].total_loads << endl
         << "Core_" << cpu << "_total_none_loads_went_offchip " << ooo_cpu[cpu].load_per_rob_part_stats[NONE].loads_went_offchip << endl
         << endl;

    // print stats for top-90% off-chip load-generating PCs
    cout << "[Histogram of top-90\% off-chip load-generating PCs]" << endl;
    cout << "pc|total_loads|loads_went_offchip|loads_went_offchip_pos[0]|loads_went_offchip_pos[1]|..." << endl;
    uint64_t load_ips = 0, frontal_bias_load_ips[5] = {0}, dorsal_bias_load_ips[5] = {0};
    float bias_values[5] = {0.9, 0.8, 0.7, 0.6, 0.5};
    uint64_t frontal_bias_load_ips_total_loads[5] = {0}, 
             frontal_bias_load_ips_off_chip_loads[5] = {0}, 
             dorsal_bias_load_ips_total_loads[5] = {0}, 
             dorsal_bias_load_ips_off_chip_loads[5] = {0};
    for(auto it = pairs.begin(); it != pairs.end(); ++it)
    {
        cout << setw(10) << hex << it->first << dec << "|"
             << setw(10) << it->second.total_loads << "|"
             << setw(10) << it->second.loads_went_offchip << "|"
            ;
        for(uint32_t i = 0; i < it->second.loads_went_offchip_pos_hist.size(); ++i)
        {
            cout << setw(7) << std::fixed << std::setprecision(2) << (float)it->second.loads_went_offchip_pos_hist[i]/it->second.loads_went_offchip*100 << "|";
        }
        cout << endl;

        load_ips++;
        
        // count frontal and dorsal offchip loads
        uint64_t frontal_loads_went_offchip = 0, dorsal_loads_went_offchip = 0;
        for(uint32_t index = 0; index < knob::num_rob_partitions; ++index)
        {
            if(ooo_cpu[cpu].rob_part_id_is_frontal(index)) 
                frontal_loads_went_offchip += it->second.loads_went_offchip_pos_hist[index];
            if(ooo_cpu[cpu].rob_part_id_is_dorsal(index)) 
                dorsal_loads_went_offchip += it->second.loads_went_offchip_pos_hist[index];
        }
        
        for(uint32_t i = 0; i < 5; ++i)
        {
            // check frontal bias
            if((float)frontal_loads_went_offchip/it->second.loads_went_offchip >= bias_values[i])
            {
                frontal_bias_load_ips[i]++;
                // keep track of total number of loads and off-chip loads generated by all frontal-biased IPs
                frontal_bias_load_ips_total_loads[i] += it->second.total_loads;
                frontal_bias_load_ips_off_chip_loads[i] += it->second.loads_went_offchip;
            }

            // check dorsal bias
            if((float)dorsal_loads_went_offchip/it->second.loads_went_offchip >= bias_values[i])
            {
                dorsal_bias_load_ips[i]++;
                // keep track of total number of loads and off-chip loads generated by all frontal-biased IPs
                dorsal_bias_load_ips_total_loads[i] += it->second.total_loads;
                dorsal_bias_load_ips_off_chip_loads[i] += it->second.loads_went_offchip;
            }
        }
        
        accum += it->second.loads_went_offchip;
        if((float)accum/total_loads_went_offchip >= 0.9)
        {
            break;
        }
    }
    cout << endl;

    cout << "Core_" << cpu << "_total_load_ips_went_offchip " << load_ips << endl;
    for(uint32_t i = 0; i < 5; ++i)
    {
        cout << "Core_" << cpu << "_load_ips_biased_" << bias_values[i] << " " << (frontal_bias_load_ips[i]+dorsal_bias_load_ips[i]) << endl
             << "Core_" << cpu << "_load_ips_frontal_biased_" << bias_values[i] << " " << frontal_bias_load_ips[i] << endl
             << "Core_" << cpu << "_load_ips_frontal_biased_" << bias_values[i] << "_total_loads " << frontal_bias_load_ips_total_loads[i] << endl
             << "Core_" << cpu << "_load_ips_frontal_biased_" << bias_values[i] << "_off_chip_loads " << frontal_bias_load_ips_off_chip_loads[i] << endl
             << "Core_" << cpu << "_load_ips_dorsal_biased_" << bias_values[i] << " " << dorsal_bias_load_ips[i] << endl
             << "Core_" << cpu << "_load_ips_dorsal_biased_" << bias_values[i] << "_total_loads " << dorsal_bias_load_ips_total_loads[i] << endl
             << "Core_" << cpu << "_load_ips_dorsal_biased_" << bias_values[i] << "_off_chip_loads " << dorsal_bias_load_ips_off_chip_loads[i] << endl;
    }
    cout << endl;

    // frontal loads per ip stats
    std::vector<std::pair<uint64_t, load_per_ip_info_t>> pairs2;
    for(auto it = ooo_cpu[cpu].frontal_load_per_ip_stats.begin(); it != ooo_cpu[cpu].frontal_load_per_ip_stats.end(); ++it)
        pairs2.push_back(*it);
    std::sort(pairs2.begin(), pairs2.end(), 
        [](const std::pair<uint64_t, load_per_ip_info_t> &p1, const std::pair<uint64_t, load_per_ip_info_t> &p2)
        {
            return p1.second.total_loads > p2.second.total_loads;
        });
    uint64_t total_frontal_loads = 0;
    for(auto it = pairs2.begin(); it != pairs2.end(); ++it)
    {
        total_frontal_loads += it->second.total_loads;
    }
    accum = 0;
    uint64_t frontal_load_generating_ips = 0, frontal_load_generating_ips_offchip_biased[5] = {0};
    for(auto it = pairs2.begin(); it != pairs2.end(); ++it)
    {
        frontal_load_generating_ips++;
        for(uint32_t i = 0; i < 5; ++i)
        {
            if((float)it->second.loads_went_offchip/it->second.total_loads >= bias_values[i])
            {
                frontal_load_generating_ips_offchip_biased[i]++;
            }
        }

        accum += it->second.total_loads;
        if((float)accum/total_frontal_loads >= 0.9)
        {
            break;
        }
    }
    cout << "Core_" << cpu << "_total_frontal_load_generating_ips " << frontal_load_generating_ips << endl;
    for(uint32_t i = 0; i < 5; ++i)
    {
        cout << "Core_" << cpu << "_frontal_load_generating_ips_offchip_biased_" << bias_values[i] << " " << frontal_load_generating_ips_offchip_biased[i] << endl;
    }
    cout << endl;

    // OFFCHIP PREDICTOR STATS
    ooo_cpu[cpu].dump_stats_offchip_predictor();

    cout << "Core_" << cpu << "_DDRP_total " << ooo_cpu[cpu].stats.ddrp.total << endl
         << "Core_" << cpu << "_DDRP_issued_after_direct_translation " << ooo_cpu[cpu].stats.ddrp.issued[0] << endl
         << "Core_" << cpu << "_DDRP_issued_after_merged_translation " << ooo_cpu[cpu].stats.ddrp.issued[1] << endl
         << "Core_" << cpu << "_DDRP_dram_RQ_full " << ooo_cpu[cpu].stats.ddrp.dram_rq_full << endl
         << "Core_" << cpu << "_DDRP_dram_MSHR_full " << ooo_cpu[cpu].stats.ddrp.dram_mshr_full << endl
         << endl;

    // DDRP MONITOR STATS
    if(ooo_cpu[cpu].ddrp_monitor) 
        ooo_cpu[cpu].ddrp_monitor->dump_stats();
}

void print_roi_stats(uint32_t cpu, CACHE *cache)
{
    uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

    for (uint32_t i=0; i<NUM_TYPES; i++) {
        TOTAL_ACCESS += cache->roi_access[cpu][i];
        TOTAL_HIT += cache->roi_hit[cpu][i];
        TOTAL_MISS += cache->roi_miss[cpu][i];
    }

    cout<< "Core_" << cpu << "_" << cache->NAME << "_total_access " << TOTAL_ACCESS << endl
        << "Core_" << cpu << "_" << cache->NAME << "_total_hit " << TOTAL_HIT << endl
        << "Core_" << cpu << "_" << cache->NAME << "_total_miss " << TOTAL_MISS << endl
        << "Core_" << cpu << "_" << cache->NAME << "_loads " << cache->roi_access[cpu][0] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_load_hit " << cache->roi_hit[cpu][0] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_load_miss " << cache->roi_miss[cpu][0] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_RFOs " << cache->roi_access[cpu][1] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_RFO_hit " << cache->roi_hit[cpu][1] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_RFO_miss " << cache->roi_miss[cpu][1] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetches " << cache->roi_access[cpu][2] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_hit " << cache->roi_hit[cpu][2] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_miss " << cache->roi_miss[cpu][2] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_writebacks " << cache->roi_access[cpu][3] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_writeback_hit " << cache->roi_hit[cpu][3] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_writeback_miss " << cache->roi_miss[cpu][3] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_requested " << cache->pf_requested << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_dropped " << cache->pf_dropped << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_issued " << cache->pf_issued << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_filled " << cache->pf_filled << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_useful " << cache->pf_useful << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_useless " << cache->pf_useless << endl
        << "Core_" << cpu << "_" << cache->NAME << "_prefetch_late " << cache->pf_late << endl
        << "Core_" << cpu << "_" << cache->NAME << "_average_miss_latency " << (1.0*(cache->total_miss_latency))/TOTAL_MISS << endl
        << endl
        << "Core_" << cpu << "_" << cache->NAME << "_rq_access " << cache->RQ->ACCESS << endl
        << "Core_" << cpu << "_" << cache->NAME << "_rq_forward " << cache->RQ->FORWARD << endl
        << "Core_" << cpu << "_" << cache->NAME << "_rq_merged " << cache->RQ->MERGED << endl
        << "Core_" << cpu << "_" << cache->NAME << "_rq_to_cache " << cache->RQ->TO_CACHE << endl
        << "Core_" << cpu << "_" << cache->NAME << "_rq_full " << cache->RQ->FULL << endl
        << endl
        << "Core_" << cpu << "_" << cache->NAME << "_wq_access " << cache->WQ.ACCESS << endl
        << "Core_" << cpu << "_" << cache->NAME << "_wq_forward " << cache->WQ.FORWARD << endl
        << "Core_" << cpu << "_" << cache->NAME << "_wq_merged " << cache->WQ.MERGED << endl
        << "Core_" << cpu << "_" << cache->NAME << "_wq_to_cache " << cache->WQ.TO_CACHE << endl
        << "Core_" << cpu << "_" << cache->NAME << "_wq_full " << cache->WQ.FULL << endl
        << endl
        << "Core_" << cpu << "_" << cache->NAME << "_pq_access " << cache->PQ.ACCESS << endl
        << "Core_" << cpu << "_" << cache->NAME << "_pq_forward " << cache->PQ.FORWARD << endl
        << "Core_" << cpu << "_" << cache->NAME << "_pq_merged " << cache->PQ.MERGED << endl
        << "Core_" << cpu << "_" << cache->NAME << "_pq_to_cache " << cache->PQ.TO_CACHE << endl
        << "Core_" << cpu << "_" << cache->NAME << "_pq_full " << cache->PQ.FULL << endl
        << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_total " << cache->stats.eviction.total << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_atleast_one_reuse " << cache->stats.eviction.atleast_one_reuse << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_atleast_one_load_reuse " << cache->stats.eviction.atleast_one_reuse_cat[LOAD] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_atleast_one_RFO_reuse " << cache->stats.eviction.atleast_one_reuse_cat[RFO] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_atleast_one_prefetch_reuse " << cache->stats.eviction.atleast_one_reuse_cat[PREFETCH] << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_atleast_one_writeback_reuse " << cache->stats.eviction.atleast_one_reuse_cat[WRITEBACK] << endl
        << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_all_reuse_total " << cache->stats.eviction.all_reuse_total << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_all_reuse_max " << cache->stats.eviction.all_reuse_max << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_all_reuse_min " << cache->stats.eviction.all_reuse_min << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_all_reuse_avg " << (float)cache->stats.eviction.all_reuse_total / cache->stats.eviction.atleast_one_reuse << endl
        ;
    
#ifdef PRINT_AUX_STATS
    for(uint32_t type = LOAD; type < NUM_TYPES; ++type)
    {
        cout << "Core_" << cpu << "_" << cache->NAME << "_eviction_" << type_names[type] <<  "_reuse_total " << cache->stats.eviction.cat_reuse_total[type] << endl
            << "Core_" << cpu << "_" << cache->NAME << "_eviction_" << type_names[type] <<  "_reuse_max " << cache->stats.eviction.cat_reuse_max[type] << endl
            << "Core_" << cpu << "_" << cache->NAME << "_eviction_" << type_names[type] <<  "_reuse_min " << cache->stats.eviction.cat_reuse_min[type] << endl
            << "Core_" << cpu << "_" << cache->NAME << "_eviction_" << type_names[type] <<  "_reuse_avg " << (float)cache->stats.eviction.cat_reuse_total[type] / cache->stats.eviction.atleast_one_reuse_cat[type] << endl
            ;
    }

    cout << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_reuse_only_frontal " << cache->stats.eviction.reuse_only_frontal << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_reuse_only_dorsal " << cache->stats.eviction.reuse_only_dorsal << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_reuse_only_none " << cache->stats.eviction.reuse_only_none << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_reuse_mixed " << cache->stats.eviction.reuse_mixed << endl
        << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_all_total " << cache->stats.eviction.dep_all_total << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_all_max " << cache->stats.eviction.dep_all_max << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_all_min " << cache->stats.eviction.dep_all_min << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_branch_mispred_total " << cache->stats.eviction.dep_branch_mispred_total << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_branch_mispred_max " << cache->stats.eviction.dep_branch_mispred_max << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_branch_mispred_min " << cache->stats.eviction.dep_branch_mispred_min << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_branch_total " << cache->stats.eviction.dep_branch_total << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_branch_max " << cache->stats.eviction.dep_branch_max << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_branch_min " << cache->stats.eviction.dep_branch_min << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_load_total " << cache->stats.eviction.dep_load_total << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_load_max " << cache->stats.eviction.dep_load_max << endl
        << "Core_" << cpu << "_" << cache->NAME << "_eviction_dep_load_min " << cache->stats.eviction.dep_load_min << endl
        << endl;

    std::vector<std::pair<uint64_t, uint64_t>> pairs;
    for(auto it = cache->dependent_map.begin(); it != cache->dependent_map.end(); ++it)
        pairs.push_back(*it);
    std::sort(pairs.begin(), pairs.end(), [](const std::pair<uint64_t, uint64_t> &a, const std::pair<uint64_t, uint64_t> &b){return a.first > b.first;});
    uint64_t total = 0;
    for(uint32_t index = 0; index < pairs.size(); ++index)
    {
        total += pairs[index].second;
    }
    uint64_t sum = 0;
    for(uint32_t index = 0; index < pairs.size(); ++index)
    {
        sum += pairs[index].second;
        if(index == 2 || index == 4 || index == 9)
        {
            cout << "Core_" << cpu << "_" << cache->NAME << "_" << (index+1) << "_highest_dependent_contri " << (float)sum/total*100 << endl;
        }
    }
    cout << endl;

    // bucketize dependence
    uint64_t dep_bucket[5] = {0,0,0,0,0};
    for(int index = pairs.size()-1; index >= 0; --index) // pairs array is already sorted, hence the reverse order
    {
        if(pairs[index].first <= 0)         dep_bucket[0] += pairs[index].second;
        else if(pairs[index].first <= 2)    dep_bucket[1] += pairs[index].second;
        else if(pairs[index].first <= 5)    dep_bucket[2] += pairs[index].second;
        else if(pairs[index].first <= 9)    dep_bucket[3] += pairs[index].second;
        else                                dep_bucket[4] += pairs[index].second;
    }
    cout << "Core_" << cpu << "_" << cache->NAME << "_dep_0 " << (float)dep_bucket[0]/cache->stats.eviction.atleast_one_reuse*100 << endl
        << "Core_" << cpu << "_" << cache->NAME << "_dep_1_2 " << (float)dep_bucket[1]/cache->stats.eviction.atleast_one_reuse*100 << endl
        << "Core_" << cpu << "_" << cache->NAME << "_dep_3_5 " << (float)dep_bucket[2]/cache->stats.eviction.atleast_one_reuse*100 << endl
        << "Core_" << cpu << "_" << cache->NAME << "_dep_6_9 " << (float)dep_bucket[3]/cache->stats.eviction.atleast_one_reuse*100 << endl
        << "Core_" << cpu << "_" << cache->NAME << "_dep_10_above " << (float)dep_bucket[4]/cache->stats.eviction.atleast_one_reuse*100 << endl
        << endl;

    cout << "[Dependence historgram]" << endl;
    cout << "[Highest-10]" << endl;
    uint32_t count = 0;
    for(auto it = pairs.begin(); it != pairs.end(); ++it)
    {
        cout << (*it).first << "," << (*it).second << endl;
        if(++count == 10) break;
    }
    cout << endl;
    cout << "[Lowest-10]" << endl;
    count = 0;
    for(auto it = pairs.rbegin(); it != pairs.rend(); ++it)
    {
        cout << (*it).first << "," << (*it).second << endl;
        if(++count == 10) break;
    }
    cout << endl;

    if(cache->cache_type == IS_LLC)
    {
        uint64_t total_load_miss = 0;
        for(uint32_t index = 0; index < ROB_SIZE; ++index)
            total_load_miss += cache->missing_load_rob_pos_hist[index];

        cout << "Core_" << cpu << "_" << cache->NAME << "_missing_load_rob_pos_hist_total " << total_load_miss << endl;
        for(uint32_t index = 0; index < ROB_SIZE; ++index)
        {
            cout << "Core_" << cpu << "_" << cache->NAME << "_missing_load_rob_pos_hist_" << index << " " << cache->missing_load_rob_pos_hist[index] << endl;
        }
        cout << endl;

        // partitioned dump
        uint32_t part_id = 0;
        sum = 0;
        for(uint32_t index = 0; index < ROB_SIZE; ++index)
        {
            if(!knob::rob_partition_boundaries.empty() && (int32_t)index == knob::rob_partition_boundaries[part_id])
            {
                cout << "Core_" << cpu << "_" << cache->NAME 
                     << "_missing_load_rob_pos_hist_part_" << part_id << " " << sum << endl;
                cout << "Core_" << cpu << "_" << cache->NAME
                     << "_missing_load_rob_pos_hist_part_" << part_id << "_perc "
                     << 100*(float)sum/total_load_miss << endl;
                sum = 0;
                part_id++;
            }
            sum += cache->missing_load_rob_pos_hist[index];
        }
        cout << "Core_" << cpu << "_" << cache->NAME 
             << "_missing_load_rob_pos_hist_part_" << part_id << " " << sum << endl;
        cout << "Core_" << cpu << "_" << cache->NAME
             << "_missing_load_rob_pos_hist_part_" << part_id << "_perc " 
             << 100*(float)sum/total_load_miss << endl;
        cout << endl;
    }
#endif
    
    cout<< "Core_" << cpu << "_" << cache->NAME << "_acc_epochs " << cache->total_acc_epochs << endl;
    for(uint32_t i = 0; i < CACHE_ACC_LEVELS; ++i)
        cout<< "Core_" << cpu << "_" << cache->NAME << "_acc_level_" << i << " " << cache->acc_epoch_hist[i] << endl;
    cout << endl;

    // print queue stats
    cout << "[Core_" << cpu << "_" << cache->NAME << "_RQ]" << endl;
    cache->RQ->dump_stats();
}

void print_sim_stats(uint32_t cpu, CACHE *cache)
{
    uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

    for (uint32_t i=0; i<NUM_TYPES; i++) {
        TOTAL_ACCESS += cache->sim_access[cpu][i];
        TOTAL_HIT += cache->sim_hit[cpu][i];
        TOTAL_MISS += cache->sim_miss[cpu][i];
    }

    cout<< "Total_stats_Core_" << cpu << "_" << cache->NAME << "_total_access " << TOTAL_ACCESS << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_total_hit " << TOTAL_HIT << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_total_miss " << TOTAL_MISS << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_loads " << cache->sim_access[cpu][0] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_load_hit " << cache->sim_hit[cpu][0] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_load_miss " << cache->sim_miss[cpu][0] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_RFOs " << cache->sim_access[cpu][1] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_RFO_hit " << cache->sim_hit[cpu][1] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_RFO_miss " << cache->sim_miss[cpu][1] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_prefetches " << cache->sim_access[cpu][2] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_prefetch_hit " << cache->sim_hit[cpu][2] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_prefetch_miss " << cache->sim_miss[cpu][2] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_writebacks " << cache->sim_access[cpu][3] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_writeback_hit " << cache->sim_hit[cpu][3] << endl
        << "Total_stats_Core_" << cpu << "_" << cache->NAME << "_writeback_miss " << cache->sim_miss[cpu][3] << endl
        << endl;
}

void print_branch_stats()
{
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << "Core_" << i << "_branch_pred_accuracy " << (100.0*(ooo_cpu[i].num_branch - ooo_cpu[i].branch_mispredictions)) / ooo_cpu[i].num_branch << endl
            << "Core_" << i << "_branch_pred_mpki " << (1000.0*ooo_cpu[i].branch_mispredictions)/(ooo_cpu[i].num_retired - ooo_cpu[i].warmup_instructions) << endl
            << "Core_" << i << "_avg_ROB_occupancy_at_mispred " << (1.0*ooo_cpu[i].total_rob_occupancy_at_branch_mispredict)/ooo_cpu[i].branch_mispredictions << endl
            << endl
            << "Core_" << i << "_not_branch " << ooo_cpu[i].total_branch_types[0] << endl
            << "Core_" << i << "_branch_direct_jump " << ooo_cpu[i].total_branch_types[1] << endl
            << "Core_" << i << "_branch_indirect " << ooo_cpu[i].total_branch_types[2] << endl
            << "Core_" << i << "_branch_conditional " << ooo_cpu[i].total_branch_types[3] << endl
            << "Core_" << i << "_branch_direct_call " << ooo_cpu[i].total_branch_types[4] << endl
            << "Core_" << i << "_branch_indirect_call " << ooo_cpu[i].total_branch_types[5] << endl
            << "Core_" << i << "_branch_return " << ooo_cpu[i].total_branch_types[6] << endl
            << "Core_" << i << "_branch_other " << ooo_cpu[i].total_branch_types[7] << endl
            << endl;
    //     cout << endl << "CPU " << i << " Branch Prediction Accuracy: ";
    //     cout << (100.0*(ooo_cpu[i].num_branch - ooo_cpu[i].branch_mispredictions)) / ooo_cpu[i].num_branch;
    //     cout << "% MPKI: " << (1000.0*ooo_cpu[i].branch_mispredictions)/(ooo_cpu[i].num_retired - ooo_cpu[i].warmup_instructions);
	// cout << " Average ROB Occupancy at Mispredict: " << (1.0*ooo_cpu[i].total_rob_occupancy_at_branch_mispredict)/ooo_cpu[i].branch_mispredictions << endl << endl;
	
	// cout << "Branch types" << endl;
	// cout << "NOT_BRANCH: " << ooo_cpu[i].total_branch_types[0] << " " << (100.0*ooo_cpu[i].total_branch_types[0])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	// cout << "BRANCH_DIRECT_JUMP: " << ooo_cpu[i].total_branch_types[1] << " " << (100.0*ooo_cpu[i].total_branch_types[1])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	// cout << "BRANCH_INDIRECT: " << ooo_cpu[i].total_branch_types[2] << " " << (100.0*ooo_cpu[i].total_branch_types[2])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	// cout << "BRANCH_CONDITIONAL: " << ooo_cpu[i].total_branch_types[3] << " " << (100.0*ooo_cpu[i].total_branch_types[3])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	// cout << "BRANCH_DIRECT_CALL: " << ooo_cpu[i].total_branch_types[4] << " " << (100.0*ooo_cpu[i].total_branch_types[4])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	// cout << "BRANCH_INDIRECT_CALL: " << ooo_cpu[i].total_branch_types[5] << " " << (100.0*ooo_cpu[i].total_branch_types[5])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	// cout << "BRANCH_RETURN: " << ooo_cpu[i].total_branch_types[6] << " " << (100.0*ooo_cpu[i].total_branch_types[6])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl;
	// cout << "BRANCH_OTHER: " << ooo_cpu[i].total_branch_types[7] << " " << (100.0*ooo_cpu[i].total_branch_types[7])/(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) << "%" << endl << endl;
    }
}

void print_dram_stats()
{
    // cout << endl;
    // cout << "DRAM Statistics" << endl;
    for (uint32_t i=0; i<DRAM_CHANNELS; i++)
    {
        cout << "Channel_" << i << "_RQ_row_buffer_hit " << uncore.DRAM.RQ[i].ROW_BUFFER_HIT << endl
            << "Channel_" << i << "_RQ_row_buffer_miss " << uncore.DRAM.RQ[i].ROW_BUFFER_MISS << endl
            << "Channel_" << i << "_WQ_row_buffer_hit " << uncore.DRAM.WQ[i].ROW_BUFFER_HIT << endl
            << "Channel_" << i << "_WQ_row_buffer_miss " << uncore.DRAM.WQ[i].ROW_BUFFER_MISS << endl
            << "Channel_" << i << "_WQ_full " << uncore.DRAM.WQ[i].FULL << endl
            << "Channel_" << i << "_dbus_congested " << uncore.DRAM.dbus_congested[i][NUM_TYPES][NUM_TYPES] << endl
            << endl;
    }

    cout << "RQ_merged " << uncore.DRAM.stats.rq_merged << endl;

    uint64_t total_congested_cycle = 0, total_congested = 0;
    for (uint32_t i=0; i<DRAM_CHANNELS; i++){
        total_congested_cycle += uncore.DRAM.dbus_cycle_congested[i];
	total_congested += uncore.DRAM.dbus_congested[i][NUM_TYPES][NUM_TYPES];
    }
    if (total_congested)
        cout << "avg_congested_cycle " << (total_congested_cycle / total_congested) << endl;
    else
        cout << "avg_congested_cycle 0" << endl;
    cout << endl;

    cout << "DRAM_total_data_loads " << uncore.DRAM.stats.data_loads.total_loads << endl
         << "DRAM_data_loads_frontal " << uncore.DRAM.stats.data_loads.load_cat[FRONTAL] << endl
         << "DRAM_data_loads_dorsal " << uncore.DRAM.stats.data_loads.load_cat[DORSAL] << endl
         << "DRAM_data_loads_none " << uncore.DRAM.stats.data_loads.load_cat[NONE] << endl
         << endl
         << "DRAM_pseudo_direct_dram_prefetch_reduced_lat " << uncore.DRAM.stats.pseudo_direct_dram_prefetch.reduced_lat << endl 
         << "DRAM_pseudo_direct_dram_prefetch_zero_lat " << uncore.DRAM.stats.pseudo_direct_dram_prefetch.zero_lat << endl
         << endl;
    
    // DDRP stats
    cout << "DRAM_DDRP_ddrp_req_total " << uncore.DRAM.stats.ddrp.ddrp_req.total << endl
         << "DRAM_DDRP_ddrp_req_went_to_dram " << uncore.DRAM.stats.ddrp.ddrp_req.went_to_dram << endl
         << "DRAM_DDRP_ddrp_req_RQ_hit_on_ddrp_req " << uncore.DRAM.stats.ddrp.ddrp_req.rq_hit[0] << endl
         << "DRAM_DDRP_ddrp_req_RQ_hit_on_llc_miss " << uncore.DRAM.stats.ddrp.ddrp_req.rq_hit[1] << endl
         << "DRAM_DDRP_ddrp_req_ddrp_buffer_hit " << uncore.DRAM.stats.ddrp.ddrp_req.ddrp_buffer_hit << endl
         << endl;
    
    for(uint32_t type = 0; type < NUM_TYPES; ++type)
    {
        cout << "DRAM_DDRP_llc_miss_" << type_names[type] << "_total " << uncore.DRAM.stats.ddrp.llc_miss.total[type]<< endl
             << "DRAM_DDRP_llc_miss_" << type_names[type] << "_went_to_dram " << uncore.DRAM.stats.ddrp.llc_miss.went_to_dram[type]<< endl
             << "DRAM_DDRP_llc_miss_" << type_names[type] << "_RQ_hit " << uncore.DRAM.stats.ddrp.llc_miss.rq_hit[type]<< endl
             << "DRAM_DDRP_llc_miss_" << type_names[type] << "_ddrp_buffer_hit " << uncore.DRAM.stats.ddrp.llc_miss.ddrp_buffer_hit[type]<< endl
             << endl;
    }

    cout << "DRAM_processed_request_returned_LOAD " << uncore.DRAM.stats.dram_process.returned[LOAD] << endl
         << "DRAM_processed_request_returned_RFO " << uncore.DRAM.stats.dram_process.returned[RFO] << endl
         << "DRAM_processed_request_returned_PREFETCH " << uncore.DRAM.stats.dram_process.returned[PREFETCH] << endl
         << "DRAM_processed_request_returned_WRITEBACK " << uncore.DRAM.stats.dram_process.returned[WRITEBACK] << endl
         << "DRAM_processed_request_buffered " << uncore.DRAM.stats.dram_process.buffered << endl
         << "DRAM_processed_request_not_returned " << uncore.DRAM.stats.dram_process.not_returned << endl
         << endl;

    cout << "ddrp_buffer_insert_called " << uncore.DRAM.stats.ddrp_buffer.insert.called << endl
         << "ddrp_buffer_insert_hit " << uncore.DRAM.stats.ddrp_buffer.insert.hit << endl
         << "ddrp_buffer_insert_evict " << uncore.DRAM.stats.ddrp_buffer.insert.evict << endl
         << "ddrp_buffer_insert_insert " << uncore.DRAM.stats.ddrp_buffer.insert.insert << endl
         << "ddrp_buffer_lookup_called " << uncore.DRAM.stats.ddrp_buffer.lookup.called << endl
         << "ddrp_buffer_lookup_hit " << uncore.DRAM.stats.ddrp_buffer.lookup.hit << endl
         << "ddrp_buffer_lookup_miss " << uncore.DRAM.stats.ddrp_buffer.lookup.miss << endl
         << endl;

    cout << "DRAM_bw_pochs " << uncore.DRAM.total_bw_epochs << endl;
    for(uint32_t index = 0; index < DRAM_BW_LEVELS; ++index)
    {
        cout << "DRAM_bw_level_" << index << " " << uncore.DRAM.bw_level_hist[index] << endl;
    }
}

void print_addr_translation_stats(uint32_t cpu)
{
    cout << "Core_" << cpu << "_major_fault " << major_fault[cpu] << endl
        << "Core_" << cpu << "_minor_fault " << minor_fault[cpu] << endl
        << endl;
}

void finish_warmup()
{
    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
             elapsed_minute = elapsed_second / 60,
             elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour*60;
    elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);

    // reset core latency
    // note: since re-ordering he function calls in the main simulation loop, it's no longer necessary to add
    //       extra latency for scheduling and execution, unless you want these steps to take longer than 1 cycle.
    SCHEDULING_LATENCY = 0;
    EXEC_LATENCY = 0;
    DECODE_LATENCY = 2;
    PAGE_TABLE_LATENCY = 100;
    SWAP_LATENCY = 100000;

    cout << endl;
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        cout << "Warmup complete CPU " << i << " instructions: " << ooo_cpu[i].num_retired << " cycles: " << current_core_cycle[i];
        cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

        ooo_cpu[i].begin_sim_cycle = current_core_cycle[i]; 
        ooo_cpu[i].begin_sim_instr = ooo_cpu[i].num_retired;
	
        ooo_cpu[i].reset_stats();

        // reset_cache_stats(i, &ooo_cpu[i].L1I);
        // reset_cache_stats(i, &ooo_cpu[i].L1D);
        // reset_cache_stats(i, &ooo_cpu[i].L2C);
        // reset_cache_stats(i, &uncore.LLC);

        ooo_cpu[i].L1I.reset_stats(i);
        ooo_cpu[i].L1D.reset_stats(i);
        ooo_cpu[i].L2C.reset_stats(i);
        uncore.LLC.reset_stats(i);
    }
    uncore.DRAM.reset_stats();
    
    cout << endl;

    // set actual cache latency
    for (uint32_t i=0; i<NUM_CPUS; i++) {
        ooo_cpu[i].ITLB.LATENCY = ITLB_LATENCY;
        ooo_cpu[i].DTLB.LATENCY = DTLB_LATENCY;
        ooo_cpu[i].STLB.LATENCY = STLB_LATENCY;
        ooo_cpu[i].L1I.LATENCY  = L1I_LATENCY;
        ooo_cpu[i].L1D.LATENCY  = L1D_LATENCY;
        ooo_cpu[i].L2C.LATENCY  = L2C_LATENCY;
    }
    uncore.LLC.LATENCY = LLC_LATENCY;
}

void print_deadlock(uint32_t i)
{
    cout << "DEADLOCK! CPU " << i << " instr_id: " << ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].instr_id;
    cout << " translated: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].translated;
    cout << " fetched: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].fetched;
    cout << " scheduled: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].scheduled;
    cout << " executed: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].executed;
    cout << " is_memory: " << +ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].is_memory;
    cout << " event: " << ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle;
    cout << " current: " << current_core_cycle[i] << endl;

    // print LQ entry
    cout << endl << "Load Queue Entry" << endl;
    for (uint32_t j=0; j<LQ_SIZE; j++) {
        cout << "[LQ] entry: " << j << " instr_id: " << ooo_cpu[i].LQ.entry[j].instr_id << " address: " << hex << ooo_cpu[i].LQ.entry[j].physical_address << dec << " translated: " << +ooo_cpu[i].LQ.entry[j].translated << " fetched: " << +ooo_cpu[i].LQ.entry[i].fetched << endl;
    }

    // print SQ entry
    cout << endl << "Store Queue Entry" << endl;
    for (uint32_t j=0; j<SQ_SIZE; j++) {
        cout << "[SQ] entry: " << j << " instr_id: " << ooo_cpu[i].SQ.entry[j].instr_id << " address: " << hex << ooo_cpu[i].SQ.entry[j].physical_address << dec << " translated: " << +ooo_cpu[i].SQ.entry[j].translated << " fetched: " << +ooo_cpu[i].SQ.entry[i].fetched << endl;
    }

    // print L1D MSHR entry
    PACKET_QUEUE *queue;
    queue = &ooo_cpu[i].L1D.MSHR;
    cout << endl << queue->NAME << " Entry" << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }

    // print L2C MSHR entry
    queue = &ooo_cpu[i].L2C.MSHR;
    cout << endl << queue->NAME << " Entry" << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }

    // print LLC MSHR entry
    queue = &uncore.LLC.MSHR;
    cout << endl << queue->NAME << " Entry" << endl;
    for (uint32_t j=0; j<queue->SIZE; j++) {
        cout << "[" << queue->NAME << "] entry: " << j << " instr_id: " << queue->entry[j].instr_id << " rob_index: " << queue->entry[j].rob_index;
        cout << " address: " << hex << queue->entry[j].address << " full_addr: " << queue->entry[j].full_addr << dec << " type: " << +queue->entry[j].type;
        cout << " fill_level: " << queue->entry[j].fill_level << " lq_index: " << queue->entry[j].lq_index << " sq_index: " << queue->entry[j].sq_index << endl; 
    }
    
    assert(0);
}

void signal_handler(int signal) 
{
	cout << "Caught signal: " << signal << endl;
	exit(1);
}

// log base 2 function from efectiu
int lg2(int n)
{
    int i, m = n, c = -1;
    for (i=0; m; i++) {
        m /= 2;
        c++;
    }
    return c;
}

uint64_t rotl64 (uint64_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

    assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;  // avoid undef behaviour with NDEBUG.  0 overhead for most types / compilers
    return (n<<c) | (n>>( (-c)&mask ));
}

uint64_t rotr64 (uint64_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

    assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;  // avoid undef behaviour with NDEBUG.  0 overhead for most types / compilers
    return (n>>c) | (n<<( (-c)&mask ));
}

RANDOM champsim_rand(champsim_seed);
uint64_t va_to_pa(uint32_t cpu, uint64_t instr_id, uint64_t va, uint64_t unique_vpage, uint8_t is_code)
{
#ifdef SANITY_CHECK
    if (va == 0) 
        assert(0);
#endif

    uint8_t  swap = 0;
    uint64_t high_bit_mask = rotr64(cpu, lg2(NUM_CPUS)),
             unique_va = va | high_bit_mask;
    //uint64_t vpage = unique_va >> LOG2_PAGE_SIZE,
    uint64_t vpage = unique_vpage | high_bit_mask,
             voffset = unique_va & ((1<<LOG2_PAGE_SIZE) - 1);

    // smart random number generator
    uint64_t random_ppage;

    map <uint64_t, uint64_t>::iterator pr = page_table.begin();
    map <uint64_t, uint64_t>::iterator ppage_check = inverse_table.begin();

    // check unique cache line footprint
    map <uint64_t, uint64_t>::iterator cl_check = unique_cl[cpu].find(unique_va >> LOG2_BLOCK_SIZE);
    if (cl_check == unique_cl[cpu].end()) { // we've never seen this cache line before
        unique_cl[cpu].insert(make_pair(unique_va >> LOG2_BLOCK_SIZE, 0));
        num_cl[cpu]++;
    }
    else
        cl_check->second++;

    pr = page_table.find(vpage);
    if (pr == page_table.end()) { // no VA => PA translation found 

        if (allocated_pages >= DRAM_PAGES) { // not enough memory

            // TODO: elaborate page replacement algorithm
            // here, ChampSim randomly selects a page that is not recently used and we only track 32K recently accessed pages
            uint8_t  found_NRU = 0;
            uint64_t NRU_vpage = 0; // implement it
            // map <uint64_t, uint64_t>::iterator pr2 = recent_page.begin();
            for (pr = page_table.begin(); pr != page_table.end(); pr++) {

                NRU_vpage = pr->first;
                if (recent_page.find(NRU_vpage) == recent_page.end()) {
                    found_NRU = 1;
                    break;
                }
            }
#ifdef SANITY_CHECK
            if (found_NRU == 0)
                assert(0);

            if (pr == page_table.end())
                assert(0);
#endif
            DP ( if (warmup_complete[cpu]) {
            cout << "[SWAP] update page table NRU_vpage: " << hex << pr->first << " new_vpage: " << vpage << " ppage: " << pr->second << dec << endl; });

            // update page table with new VA => PA mapping
            // since we cannot change the key value already inserted in a map structure, we need to erase the old node and add a new node
            uint64_t mapped_ppage = pr->second;
            page_table.erase(pr);
            page_table.insert(make_pair(vpage, mapped_ppage));

            // update inverse table with new PA => VA mapping
            ppage_check = inverse_table.find(mapped_ppage);
#ifdef SANITY_CHECK
            if (ppage_check == inverse_table.end())
                assert(0);
#endif
            ppage_check->second = vpage;

            DP ( if (warmup_complete[cpu]) {
            cout << "[SWAP] update inverse table NRU_vpage: " << hex << NRU_vpage << " new_vpage: ";
            cout << ppage_check->second << " ppage: " << ppage_check->first << dec << endl; });

            // update page_queue
            page_queue.pop();
            page_queue.push(vpage);

            // invalidate corresponding vpage and ppage from the cache hierarchy
            ooo_cpu[cpu].ITLB.invalidate_entry(NRU_vpage);
            ooo_cpu[cpu].DTLB.invalidate_entry(NRU_vpage);
            ooo_cpu[cpu].STLB.invalidate_entry(NRU_vpage);
            for (uint32_t i=0; i<BLOCK_SIZE; i++) {
                uint64_t cl_addr = (mapped_ppage << 6) | i;
                ooo_cpu[cpu].L1I.invalidate_entry(cl_addr);
                ooo_cpu[cpu].L1D.invalidate_entry(cl_addr);
                ooo_cpu[cpu].L2C.invalidate_entry(cl_addr);
                uncore.LLC.invalidate_entry(cl_addr);
            }

            // swap complete
            swap = 1;
        } else {
            uint8_t fragmented = 0;
            if (num_adjacent_page > 0)
                random_ppage = ++previous_ppage;
            else {
                random_ppage = champsim_rand.draw_rand();
                fragmented = 1;
            }

            // encoding cpu number 
            // this allows ChampSim to run homogeneous multi-programmed workloads without VA => PA aliasing
            // (e.g., cpu0: astar  cpu1: astar  cpu2: astar  cpu3: astar...)
            //random_ppage &= (~((NUM_CPUS-1)<< (32-LOG2_PAGE_SIZE)));
            //random_ppage |= (cpu<<(32-LOG2_PAGE_SIZE)); 

            while (1) { // try to find an empty physical page number
                ppage_check = inverse_table.find(random_ppage); // check if this page can be allocated 
                if (ppage_check != inverse_table.end()) { // random_ppage is not available
                    DP ( if (warmup_complete[cpu]) {
                    cout << "vpage: " << hex << ppage_check->first << " is already mapped to ppage: " << random_ppage << dec << endl; }); 
                    
                    if (num_adjacent_page > 0)
                        fragmented = 1;

                    // try one more time
                    random_ppage = champsim_rand.draw_rand();
                    
                    // encoding cpu number 
                    //random_ppage &= (~((NUM_CPUS-1)<<(32-LOG2_PAGE_SIZE)));
                    //random_ppage |= (cpu<<(32-LOG2_PAGE_SIZE)); 
                }
                else
                    break;
            }

            // insert translation to page tables
            //printf("Insert  num_adjacent_page: %u  vpage: %lx  ppage: %lx\n", num_adjacent_page, vpage, random_ppage);
            page_table.insert(make_pair(vpage, random_ppage));
            inverse_table.insert(make_pair(random_ppage, vpage));
            page_queue.push(vpage);
            previous_ppage = random_ppage;
            num_adjacent_page--;
            num_page[cpu]++;
            allocated_pages++;

            // try to allocate pages contiguously
            if (fragmented) {
                num_adjacent_page = 1 << (rand() % 10);
                DP ( if (warmup_complete[cpu]) {
                cout << "Recalculate num_adjacent_page: " << num_adjacent_page << endl; });
            }
        }

        if (swap)
            major_fault[cpu]++;
        else
            minor_fault[cpu]++;
    }
    else {
        //printf("Found  vpage: %lx  random_ppage: %lx\n", vpage, pr->second);
    }

    pr = page_table.find(vpage);
#ifdef SANITY_CHECK
    if (pr == page_table.end())
        assert(0);
#endif
    uint64_t ppage = pr->second;

    uint64_t pa = ppage << LOG2_PAGE_SIZE;
    pa |= voffset;

    DP ( if (warmup_complete[cpu]) {
    cout << "[PAGE_TABLE] instr_id: " << instr_id << " vpage: " << hex << vpage;
    cout << " => ppage: " << (pa >> LOG2_PAGE_SIZE) << " vadress: " << unique_va << " paddress: " << pa << dec << endl; });

    // as a hack for code prefetching, code translations are magical and do not pay these penalties
    if(!is_code)
      {
	// if it's data, pay these penalties
	if (swap)
	  stall_cycle[cpu] = current_core_cycle[cpu] + SWAP_LATENCY;
	else
	  stall_cycle[cpu] = current_core_cycle[cpu] + PAGE_TABLE_LATENCY;
      }

    //cout << "cpu: " << cpu << " allocated unique_vpage: " << hex << unique_vpage << " to ppage: " << ppage << dec << endl;

    return pa;
}

void cpu_l1i_prefetcher_cache_operate(uint32_t cpu_num, uint64_t v_addr, uint8_t cache_hit, uint8_t prefetch_hit)
{
  ooo_cpu[cpu_num].l1i_prefetcher_cache_operate(v_addr, cache_hit, prefetch_hit);
}

void cpu_l1i_prefetcher_cache_fill(uint32_t cpu_num, uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr)
{
  ooo_cpu[cpu_num].l1i_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr);
}

int main(int argc, char** argv)
{
	// interrupt signal hanlder
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

    cout << "**********************************************************" << endl
         << "   ChampSim Simulator. Mainline Checkout: Oct 18, 2021" << endl
         << "          Last compiled: " << __DATE__ << " " << __TIME__ << endl
         << "**********************************************************" << endl;

    // initialize knobs
    uint8_t show_heartbeat = 1;
    uint32_t seed_number = 0;

    parse_args(argc, argv);

    if(knob::cloudsuite)
    {
        MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS_SPARC;
    }

    if (knob::low_bandwidth)
        DRAM_MTPS = knob::dram_io_freq/4;
    else
        DRAM_MTPS = knob::dram_io_freq;

    // DRAM access latency
    tRP  = (uint32_t)((1.0 * tRP_DRAM_NANOSECONDS  * CPU_FREQ) / 1000); 
    tRCD = (uint32_t)((1.0 * tRCD_DRAM_NANOSECONDS * CPU_FREQ) / 1000); 
    tCAS = (uint32_t)((1.0 * tCAS_DRAM_NANOSECONDS * CPU_FREQ) / 1000); 

    // default: 16 = (64 / 8) * (3200 / 1600)
    // it takes 16 CPU cycles to tranfser 64B cache block on a 8B (64-bit) bus 
    // note that dram burst length = BLOCK_SIZE/DRAM_CHANNEL_WIDTH
    DRAM_DBUS_RETURN_TIME = (BLOCK_SIZE / DRAM_CHANNEL_WIDTH) * (1.0 * CPU_FREQ / DRAM_MTPS);
    DRAM_DBUS_MAX_CAS = DRAM_CHANNELS * (knob::measure_dram_bw_epoch / DRAM_DBUS_RETURN_TIME);

    // end consequence of knobs

    // search through the argv for "-traces"
    int found_traces = 0;
    int count_traces = 0;
    cout << endl;
    for (int i=0; i<argc; i++) {
        if (found_traces)
        {
            printf("CPU_%d runs %s\n", count_traces, argv[i]);

            sprintf(ooo_cpu[count_traces].trace_string, "%s", argv[i]);

            std::string full_name(argv[i]);
            std::string last_dot = full_name.substr(full_name.find_last_of("."));

            std::string fmtstr;
            std::string decomp_program;
            if (full_name.substr(0,4) == "http")
            {
                // Check file exists
                char testfile_command[4096];
                sprintf(testfile_command, "wget -q --spider %s", argv[i]);
                FILE *testfile = popen(testfile_command, "r");
                if (pclose(testfile))
                {
                    std::cerr << "TRACE FILE NOT FOUND" << std::endl;
                    assert(0);
                }
                fmtstr = "wget -qO- %2$s | %1$s -dc";
            }
            else
            {
                std::ifstream testfile(argv[i]);
                if (!testfile.good())
                {
                    std::cerr << "TRACE FILE NOT FOUND" << std::endl;
                    assert(0);
                }
                fmtstr = "%1$s -dc %2$s";
            }

            if (last_dot[1] == 'g') // gzip format
                decomp_program = "gzip";
            else if (last_dot[1] == 'x') // xz
                decomp_program = "xz";
            else {
                std::cout << "ChampSim does not support traces other than gz or xz compression!" << std::endl;
                assert(0);
            }

            sprintf(ooo_cpu[count_traces].gunzip_command, fmtstr.c_str(), decomp_program.c_str(), argv[i]);

            char *pch[100];
            int count_str = 0;
            pch[0] = strtok (argv[i], " /,.-");
            while (pch[count_str] != NULL) {
                //printf ("%s %d\n", pch[count_str], count_str);
                count_str++;
                pch[count_str] = strtok (NULL, " /,.-");
            }

            //printf("max count_str: %d\n", count_str);
            //printf("application: %s\n", pch[count_str-3]);

            int j = 0;
            while (pch[count_str-3][j] != '\0') {
                seed_number += pch[count_str-3][j];
                //printf("%c %d %d\n", pch[count_str-3][j], j, seed_number);
                j++;
            }

            ooo_cpu[count_traces].trace_file = popen(ooo_cpu[count_traces].gunzip_command, "r");
            if (ooo_cpu[count_traces].trace_file == NULL) {
                printf("\n*** Trace file not found: %s ***\n\n", argv[i]);
                assert(0);
            }

            count_traces++;
            if (count_traces > NUM_CPUS) {
                printf("\n*** Too many traces for the configured number of cores ***\n\n");
                assert(0);
            }
        }
        else if(strcmp(argv[i],"-traces") == 0) {
            found_traces = 1;
        }
    }

    if (count_traces != NUM_CPUS) {
        printf("\n*** Not enough traces for the configured number of cores ***\n\n");
        assert(0);
    }
    // end trace file setup

    // TODO: can we initialize these variables from the class constructor?
    srand(seed_number);
    champsim_seed = seed_number;
    for (int i=0; i<NUM_CPUS; i++) {

        ooo_cpu[i].cpu = i; 
        ooo_cpu[i].warmup_instructions = knob::warmup_instructions;
        ooo_cpu[i].simulation_instructions = knob::simulation_instructions;
        ooo_cpu[i].begin_sim_cycle = 0; 
        ooo_cpu[i].begin_sim_instr = knob::warmup_instructions;

        // ROB
        ooo_cpu[i].ROB.cpu = i;

        // BRANCH PREDICTOR
        ooo_cpu[i].initialize_branch_predictor();

        // OFFCHIP PREDICTOR
        ooo_cpu[i].initialize_offchip_predictor(champsim_seed);

        // OFFCHIP TRACER
        if(knob::enable_offchip_tracing)
        {
            ooo_cpu[i].tracer.init_tracing(knob::offchip_trace_filename, i);
        }

        // DDRP MONITOR
        ooo_cpu[i].initialize_ddrp_monitor();

        // TLBs
        ooo_cpu[i].ITLB.cpu = i;
        ooo_cpu[i].ITLB.cache_type = IS_ITLB;
        ooo_cpu[i].ITLB.create_rq();
	    ooo_cpu[i].ITLB.MAX_READ = 2;
        ooo_cpu[i].ITLB.fill_level = FILL_L1;
        ooo_cpu[i].ITLB.extra_interface = &ooo_cpu[i].L1I;
        ooo_cpu[i].ITLB.lower_level = &ooo_cpu[i].STLB; 

        ooo_cpu[i].DTLB.cpu = i;
        ooo_cpu[i].DTLB.cache_type = IS_DTLB;
        ooo_cpu[i].DTLB.create_rq();
        //ooo_cpu[i].DTLB.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
        ooo_cpu[i].DTLB.MAX_READ = 2;
        ooo_cpu[i].DTLB.fill_level = FILL_L1;
        ooo_cpu[i].DTLB.extra_interface = &ooo_cpu[i].L1D;
        ooo_cpu[i].DTLB.lower_level = &ooo_cpu[i].STLB;

        ooo_cpu[i].STLB.cpu = i;
        ooo_cpu[i].STLB.cache_type = IS_STLB;
        ooo_cpu[i].STLB.create_rq();
        ooo_cpu[i].STLB.MAX_READ = 1;
        ooo_cpu[i].STLB.fill_level = FILL_L2;
        ooo_cpu[i].STLB.upper_level_icache[i] = &ooo_cpu[i].ITLB;
        ooo_cpu[i].STLB.upper_level_dcache[i] = &ooo_cpu[i].DTLB;

        // PRIVATE CACHE
        ooo_cpu[i].L1I.cpu = i;
        ooo_cpu[i].L1I.cache_type = IS_L1I;
        ooo_cpu[i].L1I.create_rq();
        ooo_cpu[i].L1I.MAX_READ = 2;
        ooo_cpu[i].L1I.fill_level = FILL_L1;
        ooo_cpu[i].L1I.lower_level = &ooo_cpu[i].L2C; 
        ooo_cpu[i].l1i_prefetcher_initialize();
	    ooo_cpu[i].L1I.l1i_prefetcher_cache_operate = cpu_l1i_prefetcher_cache_operate;
	    ooo_cpu[i].L1I.l1i_prefetcher_cache_fill = cpu_l1i_prefetcher_cache_fill;

        ooo_cpu[i].L1D.cpu = i;
        ooo_cpu[i].L1D.cache_type = IS_L1D;
        ooo_cpu[i].L1D.create_rq();
        ooo_cpu[i].L1D.MAX_READ = (2 > MAX_READ_PER_CYCLE) ? MAX_READ_PER_CYCLE : 2;
        ooo_cpu[i].L1D.fill_level = FILL_L1;
        ooo_cpu[i].L1D.lower_level = &ooo_cpu[i].L2C; 
        ooo_cpu[i].L1D.l1d_prefetcher_initialize();

        ooo_cpu[i].L2C.cpu = i;
        ooo_cpu[i].L2C.cache_type = IS_L2C;
        ooo_cpu[i].L2C.create_rq();
        ooo_cpu[i].L2C.MAX_READ = 1;
        ooo_cpu[i].L2C.fill_level = FILL_L2;
        ooo_cpu[i].L2C.upper_level_icache[i] = &ooo_cpu[i].L1I;
        ooo_cpu[i].L2C.upper_level_dcache[i] = &ooo_cpu[i].L1D;
        ooo_cpu[i].L2C.lower_level = &uncore.LLC;
        ooo_cpu[i].L2C.l2c_prefetcher_initialize();
        if(knob::l2c_dump_access_trace)
        {
            ooo_cpu[i].L2C.tracer.init_tracing(knob::l2c_access_trace_filename, knob::l2c_dump_access_trace_type, i);
        }
        ooo_cpu[i].L2C.init_rand_engine(champsim_seed, knob::l2c_pseudo_perfect_prob);

        // SHARED CACHE
        uncore.LLC.cache_type = IS_LLC;
        uncore.LLC.fill_level = FILL_LLC;
        uncore.LLC.create_rq();
        uncore.LLC.MAX_READ = NUM_CPUS;
        uncore.LLC.upper_level_icache[i] = &ooo_cpu[i].L2C;
        uncore.LLC.upper_level_dcache[i] = &ooo_cpu[i].L2C;
        uncore.LLC.lower_level = &uncore.DRAM;

        // OFF-CHIP DRAM
        uncore.DRAM.fill_level = FILL_DRAM;
        uncore.DRAM.upper_level_icache[i] = &uncore.LLC;
        uncore.DRAM.upper_level_dcache[i] = &uncore.LLC;
        for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
            uncore.DRAM.RQ[i].is_RQ = 1;
            uncore.DRAM.WQ[i].is_WQ = 1;
        }

        // DDRP BUFFER
        if(knob::dram_cntlr_enable_ddrp_buffer)
            uncore.DRAM.init_ddrp_buffer();

        // link DRAM controller from core for DDRP
        ooo_cpu[i].dram_controller = &uncore.DRAM;

        warmup_complete[i] = 0;
        //all_warmup_complete = NUM_CPUS;
        simulation_complete[i] = 0;
        current_core_cycle[i] = 0;
        stall_cycle[i] = 0;
        
        previous_ppage = 0;
        num_adjacent_page = 0;
        num_cl[i] = 0;
        allocated_pages = 0;
        num_page[i] = 0;
        minor_fault[i] = 0;
        major_fault[i] = 0;
    }

    uncore.LLC.llc_initialize_replacement();
    uncore.LLC.llc_prefetcher_initialize();
    if(knob::llc_dump_access_trace)
    {
        uncore.LLC.tracer.init_tracing(knob::llc_access_trace_filename, knob::llc_dump_access_trace_type, -1);
    }
    uncore.LLC.init_rand_engine(champsim_seed, knob::llc_pseudo_perfect_prob);

    print_knobs();

    // simulation entry point
    start_time = time(NULL);
    uint8_t run_simulation = 1;
    while (run_simulation) 
    {
        uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
                elapsed_minute = elapsed_second / 60,
                elapsed_hour = elapsed_minute / 60;
                elapsed_minute -= elapsed_hour*60;
                elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);

        for (int i=0; i<NUM_CPUS; i++) 
        {
            // proceed one cycle
            current_core_cycle[i]++;

            //cout << "Trying to process instr_id: " << ooo_cpu[i].instr_unique_id << " fetch_stall: " << +ooo_cpu[i].fetch_stall;
            //cout << " stall_cycle: " << stall_cycle[i] << " current: " << current_core_cycle[i] << endl;

            // core might be stalled due to page fault or branch misprediction
            if (stall_cycle[i] <= current_core_cycle[i])
            {
                // retire
                if ((ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].executed == COMPLETED) && (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle <= current_core_cycle[i]))
                {
                    ooo_cpu[i].retire_rob();
                }

                // complete 
                ooo_cpu[i].update_rob();

                // schedule
                uint32_t schedule_index = ooo_cpu[i].ROB.next_schedule;
                if ((ooo_cpu[i].ROB.entry[schedule_index].scheduled == 0) && (ooo_cpu[i].ROB.entry[schedule_index].event_cycle <= current_core_cycle[i]))
                {
                    ooo_cpu[i].schedule_instruction();
                }

                // execute
                ooo_cpu[i].execute_instruction();

                ooo_cpu[i].update_rob();

                // memory operation
                ooo_cpu[i].schedule_memory_instruction();
                ooo_cpu[i].execute_memory_instruction();

                ooo_cpu[i].update_rob();

	            // decode
                if(ooo_cpu[i].DECODE_BUFFER.occupancy > 0)
                {
                    ooo_cpu[i].decode_and_dispatch();
                }
	      
                // fetch
                ooo_cpu[i].fetch_instruction();
	      
                // read from trace
                if ((ooo_cpu[i].IFETCH_BUFFER.occupancy < ooo_cpu[i].IFETCH_BUFFER.SIZE) && (ooo_cpu[i].fetch_stall == 0))
                {
                    ooo_cpu[i].read_from_trace();
                }
	        }

            // heartbeat information
            if (show_heartbeat && (ooo_cpu[i].num_retired >= ooo_cpu[i].next_print_instruction)) 
            {
                float cumulative_ipc;
                if (warmup_complete[i])
                {
                    cumulative_ipc = (1.0*(ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr)) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle);
                }
                else
                {
                    cumulative_ipc = (1.0*ooo_cpu[i].num_retired) / current_core_cycle[i];
                }
                float heartbeat_ipc = (1.0*ooo_cpu[i].num_retired - ooo_cpu[i].last_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].last_sim_cycle);

                cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i].num_retired << " cycles: " << current_core_cycle[i];
                cout << " heartbeat IPC: " << FIXED_FLOAT(heartbeat_ipc) << " cumulative IPC: " << FIXED_FLOAT(cumulative_ipc); 
                cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;
                ooo_cpu[i].next_print_instruction += STAT_PRINTING_PERIOD;

                ooo_cpu[i].last_sim_instr = ooo_cpu[i].num_retired;
                ooo_cpu[i].last_sim_cycle = current_core_cycle[i];
            }

            // check for deadlock
            if (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].ip && (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle + DEADLOCK_CYCLE) <= current_core_cycle[i])
            {
                print_deadlock(i);
            }

            // check for warmup
            // warmup complete
            if ((warmup_complete[i] == 0) && (ooo_cpu[i].num_retired > ooo_cpu[i].warmup_instructions))
            {
                warmup_complete[i] = 1;
                all_warmup_complete++;
            }
            if (all_warmup_complete == NUM_CPUS) // this part is called only once when all cores are warmed up
            {
                all_warmup_complete++;
                finish_warmup();
            }

            /*
            if (all_warmup_complete == 0) { 
                all_warmup_complete = 1;
                finish_warmup();
            }
            if (ooo_cpu[1].num_retired > 0)
                warmup_complete[1] = 1;
            */
            
            // simulation complete
            if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0) && (ooo_cpu[i].num_retired >= (ooo_cpu[i].begin_sim_instr + ooo_cpu[i].simulation_instructions))) 
            {
                simulation_complete[i] = 1;
                ooo_cpu[i].finish_sim_instr = ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr;
                ooo_cpu[i].finish_sim_cycle = current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle;

                cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i].finish_sim_instr << " cycles: " << ooo_cpu[i].finish_sim_cycle;
                cout << " cumulative IPC: " << ((float) ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle);
                cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;

                record_roi_stats(i, &ooo_cpu[i].L1D);
                record_roi_stats(i, &ooo_cpu[i].L1I);
                record_roi_stats(i, &ooo_cpu[i].L2C);
                record_roi_stats(i, &uncore.LLC);

                all_simulation_complete++;
            }

            if (all_simulation_complete == NUM_CPUS)
                run_simulation = 0;
        }

        uncore.cycle++;
        if(knob::measure_dram_bw && uncore.cycle >= uncore.DRAM.next_bw_measure_cycle)
        {
            uint64_t this_epoch_enqueue_count = uncore.DRAM.rq_enqueue_count - uncore.DRAM.last_enqueue_count;
            uncore.DRAM.epoch_enqueue_count = (uncore.DRAM.epoch_enqueue_count/2) + this_epoch_enqueue_count;
            uint32_t quartile = ((float)100*uncore.DRAM.epoch_enqueue_count)/DRAM_DBUS_MAX_CAS;
            if(quartile <= 25)      uncore.DRAM.bw = 0;
            else if(quartile <= 50) uncore.DRAM.bw = 1;
            else if(quartile <= 75) uncore.DRAM.bw = 2;
            else                    uncore.DRAM.bw = 3;
            MYLOG("cycle %lu rq_enqueue_count %lu last_enqueue_count %lu epoch_enqueue_count %lu QUARTILE %u", uncore.cycle, uncore.DRAM.rq_enqueue_count, uncore.DRAM.last_enqueue_count, uncore.DRAM.epoch_enqueue_count, uncore.DRAM.bw);
            uncore.DRAM.last_enqueue_count = uncore.DRAM.rq_enqueue_count;
            uncore.DRAM.next_bw_measure_cycle = uncore.cycle + knob::measure_dram_bw_epoch;
            uncore.DRAM.total_bw_epochs++;
            uncore.DRAM.bw_level_hist[uncore.DRAM.bw]++;
            uncore.LLC.broadcast_bw(uncore.DRAM.bw);
            for(uint32_t i = 0; i < NUM_CPUS; ++ i) ooo_cpu[i].offchip_predictor_update_dram_bw(uncore.DRAM.bw);
        }

        // TODO: should it be backward?
        uncore.DRAM.operate();
        uncore.LLC.operate();
    }

    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time),
            elapsed_minute = elapsed_second / 60,
            elapsed_hour = elapsed_minute / 60;
            elapsed_minute -= elapsed_hour*60;
            elapsed_second -= (elapsed_hour*3600 + elapsed_minute*60);
    
    cout << endl << "ChampSim completed all CPUs" << endl;
    if (NUM_CPUS > 1) 
    {
//         cout << endl << "Total Simulation Statistics (not including warmup)" << endl;
//         for (uint32_t i=0; i<NUM_CPUS; i++) 
//         {
//             cout << "Core_" << i << "_cumulative_IPC " << (float) (ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr) / (current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle) << endl
//                 << "Core_" << i << "_total_instructions " << ooo_cpu[i].num_retired - ooo_cpu[i].begin_sim_instr << endl
//                 << "Core_" << i << "_cycles " << current_core_cycle[i] - ooo_cpu[i].begin_sim_cycle << endl
//                 << endl;

// #ifndef CRC2_COMPILE
//             print_sim_stats(i, &ooo_cpu[i].L1D);
//             print_sim_stats(i, &ooo_cpu[i].L1I);
//             print_sim_stats(i, &ooo_cpu[i].L2C);
// 	        ooo_cpu[i].l1i_prefetcher_final_stats();
//             ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
// 	        ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
// #endif
//             print_sim_stats(i, &uncore.LLC);
//         }
//         uncore.LLC.llc_prefetcher_final_stats();
    }

    cout << endl << "Region of Interest Statistics" << endl;
    for (uint32_t i=0; i<NUM_CPUS; i++) 
    {
        cout << "Core_" << i << "_cumulative_IPC " << ((float) ooo_cpu[i].finish_sim_instr / ooo_cpu[i].finish_sim_cycle) << endl
            << "Core_" << i << "_total_instructions " << ooo_cpu[i].finish_sim_instr << endl
            << "Core_" << i << "_cycles " << ooo_cpu[i].finish_sim_cycle << endl
            << endl;
        
        print_core_roi_stats(i);    

#ifndef CRC2_COMPILE
        print_roi_stats(i, &ooo_cpu[i].L1D);
        print_roi_stats(i, &ooo_cpu[i].L1I);
        print_roi_stats(i, &ooo_cpu[i].L2C);
#endif
        print_roi_stats(i, &uncore.LLC);
        print_addr_translation_stats(i);
    }

    for (uint32_t i=0; i<NUM_CPUS; i++) 
    {
        ooo_cpu[i].l1i_prefetcher_final_stats();
        ooo_cpu[i].L1D.l1d_prefetcher_final_stats();
        ooo_cpu[i].L2C.l2c_prefetcher_final_stats();
    }

    uncore.LLC.llc_prefetcher_final_stats();

#ifndef CRC2_COMPILE
    uncore.LLC.llc_replacement_final_stats();
    print_branch_stats();
    print_dram_stats();
#endif

    for(uint32_t cpu = 0; cpu < NUM_CPUS; ++cpu)
    {
        if(knob::enable_offchip_tracing)
        {
            ooo_cpu[cpu].tracer.fini_tracing();
        }
        if(knob::l2c_dump_access_trace)
        {
            ooo_cpu[cpu].L2C.tracer.fini_tracing();
        }
    }
    if(knob::llc_dump_access_trace)
    {
        uncore.LLC.tracer.fini_tracing();
    }

    return 0;
}
