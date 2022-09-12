#include <algorithm>
#include "cache.h"
#include "set.h"
#include "ooo_cpu.h"
#include "uncore.h"

namespace knob
{
    extern string   offchip_pred_type;
    extern uint32_t semi_perfect_cache_page_buffer_size;
    extern bool     measure_cache_acc;
    extern uint32_t measure_cache_acc_epoch;

    extern bool     l2c_dump_access_trace;
    extern bool     llc_dump_access_trace;
    extern bool     track_load_hit_dependency_in_cache;
    extern bool     l1d_perfect;
    extern bool     l2c_perfect;
    extern bool     llc_perfect;
    extern bool     llc_pseudo_perfect_enable;
    extern float    llc_pseudo_perfect_prob;
    extern bool     llc_pseudo_perfect_enable_frontal;
    extern bool     llc_pseudo_perfect_enable_dorsal;
    extern bool     l2c_pseudo_perfect_enable;
    extern float    l2c_pseudo_perfect_prob;
    extern bool     l2c_pseudo_perfect_enable_frontal;
    extern bool     l2c_pseudo_perfect_enable_dorsal;
    extern bool     enable_ddrp;
    extern bool     offchip_pred_mark_merged_load;
    extern bool     enable_itlb_priority_rq;
    extern bool     enable_dtlb_priority_rq;
    extern bool     enable_stlb_priority_rq;
    extern bool     enable_l1i_priority_rq;
    extern bool     enable_l1d_priority_rq;
    extern bool     enable_l2c_priority_rq;
    extern bool     enable_llc_priority_rq;
    extern uint32_t itlb_priority_rq_priority_type;
    extern uint32_t dtlb_priority_rq_priority_type;
    extern uint32_t stlb_priority_rq_priority_type;
    extern uint32_t l1i_priority_rq_priority_type;
    extern uint32_t l1d_priority_rq_priority_type;
    extern uint32_t l2c_priority_rq_priority_type;
    extern uint32_t llc_priority_rq_priority_type;
}

uint64_t l2pf_access = 0;

void print_cache_config()
{
    cout << "itlb_set " << ITLB_SET << endl
        << "itlb_way " << ITLB_WAY << endl
        << "itlb_rq_size " << ITLB_RQ_SIZE << endl
        << "itlb_wq_size " << ITLB_WQ_SIZE << endl
        << "itlb_pq_size " << ITLB_PQ_SIZE << endl
        << "itlb_mshr_size " << ITLB_MSHR_SIZE << endl
        << "itlb_latency " << ITLB_LATENCY << endl
        << "itlb_priority_rq " << +knob::enable_itlb_priority_rq << endl
        << "itlb_priority_rq_type " << priority_name_string[knob::itlb_priority_rq_priority_type] << endl
        << endl
        << "dtlb_set " << DTLB_SET << endl
        << "dtlb_way " << DTLB_WAY << endl
        << "dtlb_rq_size " << DTLB_RQ_SIZE << endl
        << "dtlb_wq_size " << DTLB_WQ_SIZE << endl
        << "dtlb_pq_size " << DTLB_PQ_SIZE << endl
        << "dtlb_mshr_size " << DTLB_MSHR_SIZE << endl
        << "dtlb_latency " << DTLB_LATENCY << endl
        << "dtlb_priority_rq " << +knob::enable_dtlb_priority_rq << endl
        << "dtlb_priority_rq_type " << priority_name_string[knob::dtlb_priority_rq_priority_type] << endl
        << endl
        << "stlb_set " << STLB_SET << endl
        << "stlb_way " << STLB_WAY << endl
        << "stlb_rq_size " << STLB_RQ_SIZE << endl
        << "stlb_wq_size " << STLB_WQ_SIZE << endl
        << "stlb_pq_size " << STLB_PQ_SIZE << endl
        << "stlb_mshr_size " << STLB_MSHR_SIZE << endl
        << "stlb_latency " << STLB_LATENCY << endl
        << "stlb_priority_rq " << +knob::enable_stlb_priority_rq << endl
        << "stlb_priority_rq_type " << priority_name_string[knob::stlb_priority_rq_priority_type] << endl
        << endl
        << "l1i_size " << (L1I_SET*L1I_WAY*BLOCK_SIZE)/1024 << endl
        << "l1i_set " << L1I_SET << endl
        << "l1i_way " << L1I_WAY << endl
        << "l1i_rq_size " << L1I_RQ_SIZE << endl
        << "l1i_wq_size " << L1I_WQ_SIZE << endl
        << "l1i_pq_size " << L1I_PQ_SIZE << endl
        << "l1i_mshr_size " << L1I_MSHR_SIZE << endl
        << "l1i_latency " << L1I_LATENCY << endl
        << "l1i_priority_rq " << +knob::enable_l1i_priority_rq << endl
        << "l1i_priority_rq_type " << priority_name_string[knob::l1i_priority_rq_priority_type] << endl
        << endl
        << "l1d_size " << (L1D_SET*L1D_WAY*BLOCK_SIZE)/1024 << endl
        << "l1d_set " << L1D_SET << endl
        << "l1d_way " << L1D_WAY << endl
        << "l1d_rq_size " << L1D_RQ_SIZE << endl
        << "l1d_wq_size " << L1D_WQ_SIZE << endl
        << "l1d_pq_size " << L1D_PQ_SIZE << endl
        << "l1d_mshr_size " << L1D_MSHR_SIZE << endl
        << "l1d_latency " << L1D_LATENCY << endl
        << "l1d_priority_rq " << +knob::enable_l1d_priority_rq << endl
        << "l1d_priority_rq_type " << priority_name_string[knob::l1d_priority_rq_priority_type] << endl
        << endl
        << "l2c_size " << (L2C_SET*L2C_WAY*BLOCK_SIZE)/1024 << endl
        << "l2c_set " << L2C_SET << endl
        << "l2c_way " << L2C_WAY << endl
        << "l2c_rq_size " << L2C_RQ_SIZE << endl
        << "l2c_wq_size " << L2C_WQ_SIZE << endl
        << "l2c_pq_size " << L2C_PQ_SIZE << endl
        << "l2c_mshr_size " << L2C_MSHR_SIZE << endl
        << "l2c_latency " << L2C_LATENCY << endl
        << "l2c_priority_rq " << +knob::enable_l2c_priority_rq << endl
        << "l2c_priority_rq_type " << priority_name_string[knob::l2c_priority_rq_priority_type] << endl
        << endl
        << "llc_size " << (LLC_SET*LLC_WAY*BLOCK_SIZE)/1024 << endl
        << "llc_set " << LLC_SET << endl
        << "llc_way " << LLC_WAY << endl
        << "llc_rq_size " << LLC_RQ_SIZE << endl
        << "llc_wq_size " << LLC_WQ_SIZE << endl
        << "llc_pq_size " << LLC_PQ_SIZE << endl
        << "llc_mshr_size " << LLC_MSHR_SIZE << endl
        << "llc_latency " << LLC_LATENCY << endl
        << "llc_priority_rq " << +knob::enable_llc_priority_rq << endl
        << "llc_priority_rq_type " << priority_name_string[knob::llc_priority_rq_priority_type] << endl
        << endl;
}

void CACHE::create_rq()
{
    // create RQ appropriately
    bool priority_rq = false;
    uint32_t priority_type = 0;
    if (cache_type == IS_ITLB && knob::enable_itlb_priority_rq)        {priority_rq = true; priority_type = knob::itlb_priority_rq_priority_type;}
    else if (cache_type == IS_DTLB && knob::enable_dtlb_priority_rq)   {priority_rq = true; priority_type = knob::dtlb_priority_rq_priority_type;}
    else if (cache_type == IS_STLB && knob::enable_stlb_priority_rq)   {priority_rq = true; priority_type = knob::stlb_priority_rq_priority_type;}
    else if (cache_type == IS_L1I && knob::enable_l1i_priority_rq)     {priority_rq = true; priority_type = knob::l1i_priority_rq_priority_type;}
    else if (cache_type == IS_L1D && knob::enable_l1d_priority_rq)     {priority_rq = true; priority_type = knob::l1d_priority_rq_priority_type;}
    else if (cache_type == IS_L2C && knob::enable_l2c_priority_rq)     {priority_rq = true; priority_type = knob::l2c_priority_rq_priority_type;}
    else if (cache_type == IS_LLC && knob::enable_llc_priority_rq)     {priority_rq = true; priority_type = knob::llc_priority_rq_priority_type;}

    if(priority_rq)
    {
        if(priority_type == 2 || priority_type == 3)
        {
            assert(knob::offchip_pred_type != "none");
        }
        cout << "Adding priority-RQ type " << priority_type << " in " << NAME << endl;
        RQ = new PACKET_QUEUE_PRIORITY((priority_type_t)priority_type);
        RQ->init(RQ_SIZE);
        RQ->is_RQ = 1;
        RQ->NAME = NAME + "_RQ";
        RQ->deduce_module_queue_types();
    }
    else
    {
        cout << "Adding basic-RQ in " << NAME << endl;
        RQ = new PACKET_QUEUE();
        RQ->init(RQ_SIZE);
        RQ->is_RQ = 1;
        RQ->NAME = NAME + "_RQ";
        RQ->deduce_module_queue_types();
    }
}

void CACHE::handle_fill()
{
    // handle fill
    uint32_t fill_cpu = (MSHR.next_fill_index == MSHR_SIZE) ? NUM_CPUS : MSHR.entry[MSHR.next_fill_index].cpu;
    if (fill_cpu == NUM_CPUS)
    {
        return;
    }

    if (MSHR.next_fill_cycle <= current_core_cycle[fill_cpu]) 
    {

#ifdef SANITY_CHECK
        if (MSHR.next_fill_index >= MSHR.SIZE)
        {
            assert(0);
        }
#endif

        uint32_t mshr_index = MSHR.next_fill_index;

        // find victim
        uint32_t set = get_set(MSHR.entry[mshr_index].address), way;
        if (cache_type == IS_LLC) 
        {
            way = llc_find_victim(fill_cpu, MSHR.entry[mshr_index].instr_id, set, block[set], MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
        }
        else
        {
            way = find_victim(fill_cpu, MSHR.entry[mshr_index].instr_id, set, block[set], MSHR.entry[mshr_index].ip, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type);
        }

#ifdef LLC_BYPASS
        if ((cache_type == IS_LLC) && (way == LLC_WAY)) // this is a bypass that does not fill the LLC
        { 
            // update replacement policy
            if (cache_type == IS_LLC) 
            {
                llc_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);
            }
            else
            {
                update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, 0, MSHR.entry[mshr_index].type, 0);
            }

            // COLLECT STATS
            sim_miss[fill_cpu][MSHR.entry[mshr_index].type]++;
            sim_access[fill_cpu][MSHR.entry[mshr_index].type]++;

            // check fill level
            if (MSHR.entry[mshr_index].fill_level < fill_level) 
            {
                if(fill_level == FILL_L2)
		        {
                    if(MSHR.entry[mshr_index].fill_l1i)
                    {
                        upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
                    if(MSHR.entry[mshr_index].fill_l1d)
                    {
                        upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
		        }
                else
                {
                    if (MSHR.entry[mshr_index].instruction)
                    {
                        upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
                    if (MSHR.entry[mshr_index].is_data)
                    {
                        upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
                }
            }

	        if(warmup_complete[fill_cpu] && (MSHR.entry[mshr_index].cycle_enqueued != 0))
	        {
                uint64_t current_miss_latency = (current_core_cycle[fill_cpu] - MSHR.entry[mshr_index].cycle_enqueued);
                total_miss_latency += current_miss_latency;
            }

            MSHR.remove_queue(&MSHR.entry[mshr_index]);
            MSHR.num_returned--;

            update_fill_cycle();

            return; // return here, no need to process further in this function
        }
#endif

        uint8_t  do_fill = 1;

        // is this dirty?
        if (block[set][way].dirty)
        {
            // check if the lower level WQ has enough room to keep this writeback request
            if (lower_level) 
            {
                if (lower_level->get_occupancy(2, block[set][way].address) == lower_level->get_size(2, block[set][way].address)) 
                {
                    // lower level WQ is full, cannot replace this victim
                    do_fill = 0;
                    lower_level->increment_WQ_FULL(block[set][way].address);
                    STALL[MSHR.entry[mshr_index].type]++;

                    DP ( if (warmup_complete[fill_cpu]) {
                    cout << "[" << NAME << "] " << __func__ << "do_fill: " << +do_fill;
                    cout << " lower level wq is full!" << " fill_addr: " << hex << MSHR.entry[mshr_index].address;
                    cout << " victim_addr: " << block[set][way].tag << dec << endl; });
                }
                else 
                {
                    PACKET writeback_packet;

                    writeback_packet.fill_level = fill_level << 1;
                    writeback_packet.cpu = fill_cpu;
                    writeback_packet.address = block[set][way].address;
                    writeback_packet.full_addr = block[set][way].full_addr;
                    writeback_packet.data = block[set][way].data;
                    writeback_packet.instr_id = MSHR.entry[mshr_index].instr_id;
                    writeback_packet.ip = 0; // writeback does not have ip
                    writeback_packet.type = WRITEBACK;
                    writeback_packet.event_cycle = current_core_cycle[fill_cpu];

                    lower_level->add_wq(&writeback_packet);
                }
            }
#ifdef SANITY_CHECK
            else 
            {
                // sanity check
                if (cache_type != IS_STLB)
                    assert(0);
            }
#endif
        }

        if (do_fill)
        {
            // update prefetcher
            if (cache_type == IS_L1I)
            {
                l1i_prefetcher_cache_fill(fill_cpu, ((MSHR.entry[mshr_index].ip)>>LOG2_BLOCK_SIZE)<<LOG2_BLOCK_SIZE, set, way, (MSHR.entry[mshr_index].type == PREFETCH) ? 1 : 0, ((block[set][way].ip)>>LOG2_BLOCK_SIZE)<<LOG2_BLOCK_SIZE);
            }
            if (cache_type == IS_L1D)
            {
                l1d_prefetcher_cache_fill(MSHR.entry[mshr_index].full_addr, set, way, (MSHR.entry[mshr_index].type == PREFETCH) ? 1 : 0, block[set][way].address<<LOG2_BLOCK_SIZE, MSHR.entry[mshr_index].pf_metadata);
            }
            if (cache_type == IS_L2C)
            {
	            MSHR.entry[mshr_index].pf_metadata = l2c_prefetcher_cache_fill(MSHR.entry[mshr_index].address<<LOG2_BLOCK_SIZE, set, way, (MSHR.entry[mshr_index].type == PREFETCH) ? 1 : 0, block[set][way].address<<LOG2_BLOCK_SIZE, MSHR.entry[mshr_index].pf_metadata);
            }
            if (cache_type == IS_LLC)
	        {
		        cpu = fill_cpu;
		        MSHR.entry[mshr_index].pf_metadata = llc_prefetcher_cache_fill(MSHR.entry[mshr_index].address<<LOG2_BLOCK_SIZE, set, way, (MSHR.entry[mshr_index].type == PREFETCH) ? 1 : 0, block[set][way].address<<LOG2_BLOCK_SIZE, MSHR.entry[mshr_index].pf_metadata);
		        cpu = 0;
	        }
              
            // update replacement policy
            if (cache_type == IS_LLC) 
            {
                llc_update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, block[set][way].full_addr, MSHR.entry[mshr_index].type, 0);
            }
            else
            {
                update_replacement_state(fill_cpu, set, way, MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].ip, block[set][way].full_addr, MSHR.entry[mshr_index].type, 0);
            }

            // update off-chip predictor for LLC evictions
            if (cache_type == IS_LLC)
            {
                ooo_cpu[fill_cpu].offchip_predictor_track_llc_eviction(set, way, block[set][way].full_addr);
            }

            // // @RBERA: moved this code to handle_read
            // // @RBERA: if this is a load fill in LLC, then monitor the position of the load in ROB
            // if(cache_type == IS_LLC && MSHR.entry[mshr_index].type == LOAD)
            // {
                
            //     if(MSHR.entry[mshr_index].rob_position < 0 && MSHR.entry[mshr_index].rob_position >= ROB_SIZE)
            //     {
            //         cout << "invalid ROB position: index: " << mshr_index << " pos: " << MSHR.entry[mshr_index].rob_position << endl;
            //         assert(0); 
            //     }
            //     // missing_load_rob_pos_hist[MSHR.entry[mshr_index].rob_position]++;
            // }

            // COLLECT STATS
            sim_miss[fill_cpu][MSHR.entry[mshr_index].type]++;
            sim_access[fill_cpu][MSHR.entry[mshr_index].type]++;

            // RBERA-TODO: Dump cache access trace
            if(knob::l2c_dump_access_trace && cache_type == IS_L2C && warmup_complete[fill_cpu])
            {
                tracer.record_trace(MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type, false);
            }
            if(knob::llc_dump_access_trace && cache_type == IS_LLC && warmup_complete[fill_cpu])
            {
                tracer.record_trace(MSHR.entry[mshr_index].full_addr, MSHR.entry[mshr_index].type, false);
            }

            fill_cache(set, way, &MSHR.entry[mshr_index]);

            // RFO marks cache line dirty
            if (cache_type == IS_L1D)
            {
                if (MSHR.entry[mshr_index].type == RFO)
                {
                    block[set][way].dirty = 1;
                }
            }

            // send response to upper level cache
            if (MSHR.entry[mshr_index].fill_level < fill_level) 
            {
	            if(fill_level == FILL_L2)
                {
                    if(MSHR.entry[mshr_index].fill_l1i)
                    {
                        upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
                    if(MSHR.entry[mshr_index].fill_l1d)
                    {
                        upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
                }
	            else
                {
                    if (MSHR.entry[mshr_index].instruction)
                    {
                                upper_level_icache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
                    if (MSHR.entry[mshr_index].is_data)
                    {
                                upper_level_dcache[fill_cpu]->return_data(&MSHR.entry[mshr_index]);
                    }
                }
            }

            // update processed packets
            if (cache_type == IS_ITLB) 
            { 
                MSHR.entry[mshr_index].instruction_pa = block[set][way].data;
                if (PROCESSED.occupancy < PROCESSED.SIZE)
                {
                    PROCESSED.add_queue(&MSHR.entry[mshr_index], current_core_cycle[fill_cpu]);
                }
            }
            else if (cache_type == IS_DTLB) 
            {
                MSHR.entry[mshr_index].data_pa = block[set][way].data;
                if (PROCESSED.occupancy < PROCESSED.SIZE)
                {
                    PROCESSED.add_queue(&MSHR.entry[mshr_index], current_core_cycle[fill_cpu]);
                }
            }
            else if (cache_type == IS_L1I)
            {
                if (PROCESSED.occupancy < PROCESSED.SIZE)
                {
                    PROCESSED.add_queue(&MSHR.entry[mshr_index], current_core_cycle[fill_cpu]);
                }
            }
            //else if (cache_type == IS_L1D) {
            else if ((cache_type == IS_L1D) && (MSHR.entry[mshr_index].type != PREFETCH)) 
            {
                if (PROCESSED.occupancy < PROCESSED.SIZE)
                {
                    PROCESSED.add_queue(&MSHR.entry[mshr_index], current_core_cycle[fill_cpu]);
                }
            }

	        if(warmup_complete[fill_cpu] && (MSHR.entry[mshr_index].cycle_enqueued != 0))
	        {
		        uint64_t current_miss_latency = (current_core_cycle[fill_cpu] - MSHR.entry[mshr_index].cycle_enqueued);
                /*
                if(cache_type == IS_L1D)
                {
                    cout << current_core_cycle[fill_cpu] << " - " << MSHR.entry[mshr_index].cycle_enqueued << " = " << current_miss_latency << " MSHR index: " << mshr_index << endl;
                }
                */
                total_miss_latency += current_miss_latency;
	        }
	  
            MSHR.remove_queue(&MSHR.entry[mshr_index]);
            MSHR.num_returned--;

            update_fill_cycle();
        }
    }
}

void CACHE::handle_writeback()
{
    // handle write
    uint32_t writeback_cpu = WQ.entry[WQ.head].cpu;
    if (writeback_cpu == NUM_CPUS)
    {
        return;
    }

    // handle the oldest entry
    if ((WQ.entry[WQ.head].event_cycle <= current_core_cycle[writeback_cpu]) && (WQ.occupancy > 0)) 
    {
        int index = WQ.head;

        // access cache
        uint32_t set = get_set(WQ.entry[index].address);
        int way = check_hit(&WQ.entry[index]);
        
        if (way >= 0) // writeback hit (or RFO hit for L1D)
        {
            WQ.entry[index].hit_where = assign_hit_where(cache_type, 0); // writeback hit
            
            if (cache_type == IS_LLC) 
            {
                llc_update_replacement_state(writeback_cpu, set, way, block[set][way].full_addr, WQ.entry[index].ip, 0, WQ.entry[index].type, 1);
            }
            else
            {
                update_replacement_state(writeback_cpu, set, way, block[set][way].full_addr, WQ.entry[index].ip, 0, WQ.entry[index].type, 1);
            }

            // COLLECT STATS
            sim_hit[writeback_cpu][WQ.entry[index].type]++;
            sim_access[writeback_cpu][WQ.entry[index].type]++;

            // @RBERA: populate reuse metadata
            block[set][way].reuse[WQ.entry[index].type]++;

            // RBERA-TODO: Dump cache access trace
            if(knob::l2c_dump_access_trace && cache_type == IS_L2C && warmup_complete[writeback_cpu])
            {
                tracer.record_trace(WQ.entry[index].full_addr, WQ.entry[index].type, true);
            }
            if(knob::llc_dump_access_trace && cache_type == IS_LLC && warmup_complete[writeback_cpu])
            {
                tracer.record_trace(WQ.entry[index].full_addr, WQ.entry[index].type, true);
            }

            // mark dirty
            block[set][way].dirty = 1;

            if (cache_type == IS_ITLB)
            {
                WQ.entry[index].instruction_pa = block[set][way].data;
            }
            else if (cache_type == IS_DTLB)
            {
                WQ.entry[index].data_pa = block[set][way].data;
            }
            else if (cache_type == IS_STLB)
            {
                WQ.entry[index].data = block[set][way].data;
            }

            // send the response to upper level
            if (WQ.entry[index].fill_level < fill_level) 
            {
                if(fill_level == FILL_L2)
                {
                    if(WQ.entry[index].fill_l1i)
                    {
                        upper_level_icache[writeback_cpu]->return_data(&WQ.entry[index]);
                    }
                    if(WQ.entry[index].fill_l1d)
                    {
                        upper_level_dcache[writeback_cpu]->return_data(&WQ.entry[index]);
                    }
                }
                else
                {
                    if (WQ.entry[index].instruction)
                    {
                            upper_level_icache[writeback_cpu]->return_data(&WQ.entry[index]);
                    }
                    if (WQ.entry[index].is_data)
                    {
                            upper_level_dcache[writeback_cpu]->return_data(&WQ.entry[index]);
                    }
                }
            }

            // old code
            HIT[WQ.entry[index].type]++;
            ACCESS[WQ.entry[index].type]++;

            // remove this entry from WQ
            uint64_t deque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[writeback_cpu];
            WQ.remove_queue(&WQ.entry[index], deque_cycle);
        }
        else // writeback miss (or RFO miss for L1D)
        { 
            DP ( if (warmup_complete[writeback_cpu]) {
            cout << "[" << NAME << "] " << __func__ << " type: " << +WQ.entry[index].type << " miss";
            cout << " instr_id: " << WQ.entry[index].instr_id << " address: " << hex << WQ.entry[index].address;
            cout << " full_addr: " << WQ.entry[index].full_addr << dec;
            cout << " cycle: " << WQ.entry[index].event_cycle << endl; });

            if (cache_type == IS_L1D) // RFO miss
            { 
                // check mshr
                uint8_t miss_handled = 1;
                int mshr_index = check_mshr(&WQ.entry[index]);

		        if(mshr_index == -2)
		        {
		            // this is a data/instruction collision in the MSHR, so we have to wait before we can allocate this miss
		            miss_handled = 0;
		        }
                else if ((mshr_index == -1) && (MSHR.occupancy < MSHR_SIZE)) // this is a new miss
                { 
		            if(cache_type == IS_LLC)
		            {
		                // check to make sure the DRAM RQ has room for this LLC RFO miss
		                if (lower_level->get_occupancy(1, WQ.entry[index].address) == lower_level->get_size(1, WQ.entry[index].address))
                        {
                            miss_handled = 0;
                        }
                        else
                        {
                            add_mshr(&WQ.entry[index]);
                            lower_level->add_rq(&WQ.entry[index]);
                        }
		            }
                    else
                    {
                        // check to make sure the DRAM RQ has room for this LLC RFO miss
		                if (lower_level && lower_level->get_occupancy(1, WQ.entry[index].address) == lower_level->get_size(1, WQ.entry[index].address))
                        {
                            miss_handled = 0;
                        }
                        else
                        {
                            // add it to mshr (RFO miss)
                            add_mshr(&WQ.entry[index]);
                    
                            // add it to the next level's read queue
                            //if (lower_level) // L1D always has a lower level cache
                            lower_level->add_rq(&WQ.entry[index]);
                        }
                    }
                }
                else 
                {
                    if ((mshr_index == -1) && (MSHR.occupancy == MSHR_SIZE)) // not enough MSHR resource
                    { 
                        // cannot handle miss request until one of MSHRs is available
                        miss_handled = 0;
                        STALL[WQ.entry[index].type]++;
                    }
                    else if (mshr_index != -1) // already in-flight miss
                    {
                        WQ.entry[index].hit_where = assign_hit_where(cache_type, 3); // writeback hit in MSHR
                        
                        // update fill_level
                        if (WQ.entry[index].fill_level < MSHR.entry[mshr_index].fill_level)
                        {
                            MSHR.entry[mshr_index].fill_level = WQ.entry[index].fill_level;
                        }
                        if((WQ.entry[index].fill_l1i) && (MSHR.entry[mshr_index].fill_l1i != 1))
                        {
                            MSHR.entry[mshr_index].fill_l1i = 1;
                        }
                        if((WQ.entry[index].fill_l1d) && (MSHR.entry[mshr_index].fill_l1d != 1))
                        {
                            MSHR.entry[mshr_index].fill_l1d = 1;
                        }

                        // update request
                        if (MSHR.entry[mshr_index].type == PREFETCH) 
                        {
                            uint8_t  prior_returned = MSHR.entry[mshr_index].returned;
                            uint64_t prior_event_cycle = MSHR.entry[mshr_index].event_cycle;
			                MSHR.entry[mshr_index] = WQ.entry[index];

                            // in case request is already returned, we should keep event_cycle and retunred variables
                            MSHR.entry[mshr_index].returned = prior_returned;
                            MSHR.entry[mshr_index].event_cycle = prior_event_cycle;
                        }

                        MSHR_MERGED[WQ.entry[index].type]++;

                        DP ( if (warmup_complete[writeback_cpu]) {
                        cout << "[" << NAME << "] " << __func__ << " mshr merged";
                        cout << " instr_id: " << WQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id; 
                        cout << " address: " << hex << WQ.entry[index].address;
                        cout << " full_addr: " << WQ.entry[index].full_addr << dec;
                        cout << " cycle: " << WQ.entry[index].event_cycle << endl; });
                    }
                    else // WE SHOULD NOT REACH HERE
                    {
                        cerr << "[" << NAME << "] MSHR errors" << endl;
                        assert(0);
                    }
                }

                if (miss_handled) 
                {
                    MISS[WQ.entry[index].type]++;
                    ACCESS[WQ.entry[index].type]++;

                    // remove this entry from WQ
                    uint64_t deque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[writeback_cpu];
                    WQ.remove_queue(&WQ.entry[index], deque_cycle);
                }

            }
            else 
            {
                // find victim
                uint32_t set = get_set(WQ.entry[index].address), way;
                if (cache_type == IS_LLC) 
                {
                    way = llc_find_victim(writeback_cpu, WQ.entry[index].instr_id, set, block[set], WQ.entry[index].ip, WQ.entry[index].full_addr, WQ.entry[index].type);
                }
                else
                {
                    way = find_victim(writeback_cpu, WQ.entry[index].instr_id, set, block[set], WQ.entry[index].ip, WQ.entry[index].full_addr, WQ.entry[index].type);
                }

#ifdef LLC_BYPASS
                if ((cache_type == IS_LLC) && (way == LLC_WAY)) 
                {
                    cerr << "LLC bypassing for writebacks is not allowed!" << endl;
                    assert(0);
                }
#endif

                uint8_t  do_fill = 1;

                // is this dirty?
                if (block[set][way].dirty) 
                {
                    // check if the lower level WQ has enough room to keep this writeback request
                    if (lower_level) 
                    { 
                        if (lower_level->get_occupancy(2, block[set][way].address) == lower_level->get_size(2, block[set][way].address)) 
                        {
                            // lower level WQ is full, cannot replace this victim
                            do_fill = 0;
                            lower_level->increment_WQ_FULL(block[set][way].address);
                            STALL[WQ.entry[index].type]++;

                            DP ( if (warmup_complete[writeback_cpu]) {
                            cout << "[" << NAME << "] " << __func__ << "do_fill: " << +do_fill;
                            cout << " lower level wq is full!" << " fill_addr: " << hex << WQ.entry[index].address;
                            cout << " victim_addr: " << block[set][way].tag << dec << endl; });
                        }
                        else 
                        { 
                            PACKET writeback_packet;

                            writeback_packet.fill_level = fill_level << 1;
                            writeback_packet.cpu = writeback_cpu;
                            writeback_packet.address = block[set][way].address;
                            writeback_packet.full_addr = block[set][way].full_addr;
                            writeback_packet.data = block[set][way].data;
                            writeback_packet.instr_id = WQ.entry[index].instr_id;
                            writeback_packet.ip = 0;
                            writeback_packet.type = WRITEBACK;
                            writeback_packet.event_cycle = current_core_cycle[writeback_cpu];

                            lower_level->add_wq(&writeback_packet);
                        }
                    }
#ifdef SANITY_CHECK
                    else 
                    {
                        // sanity check
                        if (cache_type != IS_STLB)
                            assert(0);
                    }
#endif
                }

                if (do_fill) 
                {
                    // update prefetcher
                    // RBERA: why is the prefetcher_fill called for writeback fills?
                    // RBERA: this sounds incorrect!
		            if (cache_type == IS_L1I)
                    {
                            l1i_prefetcher_cache_fill(writeback_cpu, ((WQ.entry[index].ip)>>LOG2_BLOCK_SIZE)<<LOG2_BLOCK_SIZE, set, way, 0, ((block[set][way].ip)>>LOG2_BLOCK_SIZE)<<LOG2_BLOCK_SIZE);
                    }
                    if (cache_type == IS_L1D)
                    {
		                  l1d_prefetcher_cache_fill(WQ.entry[index].full_addr, set, way, 0, block[set][way].address<<LOG2_BLOCK_SIZE, WQ.entry[index].pf_metadata);
                    }
                    else if (cache_type == IS_L2C)
                    {
		                    WQ.entry[index].pf_metadata = l2c_prefetcher_cache_fill(WQ.entry[index].address<<LOG2_BLOCK_SIZE, set, way, 0, block[set][way].address<<LOG2_BLOCK_SIZE, WQ.entry[index].pf_metadata);
                    }
                    if (cache_type == IS_LLC)
		            {
                        cpu = writeback_cpu;
                        WQ.entry[index].pf_metadata =llc_prefetcher_cache_fill(WQ.entry[index].address<<LOG2_BLOCK_SIZE, set, way, 0, block[set][way].address<<LOG2_BLOCK_SIZE, WQ.entry[index].pf_metadata);
                        cpu = 0;
                    }

                    // update replacement policy
                    if (cache_type == IS_LLC) 
                    {
                        llc_update_replacement_state(writeback_cpu, set, way, WQ.entry[index].full_addr, WQ.entry[index].ip, block[set][way].full_addr, WQ.entry[index].type, 0);
                    }
                    else
                    {
                        update_replacement_state(writeback_cpu, set, way, WQ.entry[index].full_addr, WQ.entry[index].ip, block[set][way].full_addr, WQ.entry[index].type, 0);
                    }

                    // COLLECT STATS
                    sim_miss[writeback_cpu][WQ.entry[index].type]++;
                    sim_access[writeback_cpu][WQ.entry[index].type]++;

                    // RBERA-TODO: Dump cache access trace
                    if(knob::l2c_dump_access_trace && cache_type == IS_L2C && warmup_complete[writeback_cpu])
                    {
                        tracer.record_trace(WQ.entry[index].full_addr, WQ.entry[index].type, false);
                    }
                    if(knob::llc_dump_access_trace && cache_type == IS_LLC && warmup_complete[writeback_cpu])
                    {
                        tracer.record_trace(WQ.entry[index].full_addr, WQ.entry[index].type, false);
                    }

                    fill_cache(set, way, &WQ.entry[index]);

                    // mark dirty
                    block[set][way].dirty = 1; 

                    // check fill level
                    if (WQ.entry[index].fill_level < fill_level) 
                    {
		                if(fill_level == FILL_L2)
                        {
                            if(WQ.entry[index].fill_l1i)
                            {
                                upper_level_icache[writeback_cpu]->return_data(&WQ.entry[index]);
                            }
                            if(WQ.entry[index].fill_l1d)
                            {
                                upper_level_dcache[writeback_cpu]->return_data(&WQ.entry[index]);
                            }
                        }
                        else
                        {
                            if (WQ.entry[index].instruction)
                            {
                                upper_level_icache[writeback_cpu]->return_data(&WQ.entry[index]);
                            }
                            if (WQ.entry[index].is_data)
                            {
                                upper_level_dcache[writeback_cpu]->return_data(&WQ.entry[index]);
                            }
                        }
                    }

                    MISS[WQ.entry[index].type]++;
                    ACCESS[WQ.entry[index].type]++;

                    // remove this entry from WQ
                    uint64_t deque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[writeback_cpu];
                    WQ.remove_queue(&WQ.entry[index], deque_cycle);
                }
            }
        }
    }
}

void CACHE::handle_read()
{
    // handle read
    for (uint32_t i=0; i<MAX_READ; i++) 
    {
        if(RQ->is_empty())
        {
            return;
        }
        
        uint32_t read_cpu = RQ->peek().cpu;

        // handle the oldest entry
        if ((RQ->peek().event_cycle <= current_core_cycle[read_cpu]) && (RQ->occupancy > 0)) 
        {
            int index = RQ->get_head();
            PACKET& rq_entry = RQ->get_entry(RQ->get_head());

            // access cache
            uint32_t set = get_set(rq_entry.address);
            int way = check_hit(&rq_entry);
            
            if (way >= 0) // read hit
            {
                rq_entry.hit_where = assign_hit_where(cache_type, 0); // read hit

                if (cache_type == IS_ITLB) 
                {
                    rq_entry.instruction_pa = block[set][way].data;
                    if (PROCESSED.occupancy < PROCESSED.SIZE)
                    {
                        PROCESSED.add_queue(&rq_entry, current_core_cycle[read_cpu]);
                    }
                }
                else if (cache_type == IS_DTLB)
                {
                    rq_entry.data_pa = block[set][way].data;
                    if (PROCESSED.occupancy < PROCESSED.SIZE)
                    {
                        PROCESSED.add_queue(&rq_entry, current_core_cycle[read_cpu]);
                    }
                }
                else if (cache_type == IS_STLB) 
                {
                    rq_entry.data = block[set][way].data;
                }
                else if (cache_type == IS_L1I) 
                {
                    if (PROCESSED.occupancy < PROCESSED.SIZE)
                    {
                        PROCESSED.add_queue(&rq_entry, current_core_cycle[read_cpu]);
                    }
                }
                //else if (cache_type == IS_L1D) {
                else if ((cache_type == IS_L1D) && (rq_entry.type != PREFETCH)) 
                {
                    if (PROCESSED.occupancy < PROCESSED.SIZE)
                    {
                        PROCESSED.add_queue(&rq_entry, current_core_cycle[read_cpu]);
                    }
                }

                // update prefetcher on load instruction
		        if (rq_entry.type == LOAD) 
                {
		            if(cache_type == IS_L1I)
                    {
		                l1i_prefetcher_cache_operate(read_cpu, rq_entry.ip, 1, block[set][way].prefetch);
                    }
                    if (cache_type == IS_L1D) 
                    {
		                l1d_prefetcher_operate(rq_entry.full_addr, rq_entry.ip, 1, rq_entry.type);
                    }
                    else if (cache_type == IS_L2C)
                    {
		                l2c_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, rq_entry.ip, 1, rq_entry.type, 0);
                    }
                    else if (cache_type == IS_LLC)
                    {
                        cpu = read_cpu;
                        llc_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, rq_entry.ip, 1, rq_entry.type, 0);
                        cpu = 0;
                    }
                }

                // update replacement policy
                if (cache_type == IS_LLC) 
                {
                    llc_update_replacement_state(read_cpu, set, way, block[set][way].full_addr, rq_entry.ip, 0, rq_entry.type, 1);
                }
                else
                {
                    update_replacement_state(read_cpu, set, way, block[set][way].full_addr, rq_entry.ip, 0, rq_entry.type, 1);
                }

                // COLLECT STATS
                sim_hit[read_cpu][rq_entry.type]++;
                sim_access[read_cpu][rq_entry.type]++;

                // @RBERA: this is a load hit (actually, this can be a RFO hit also!)
                // lokup the dependency chain of the load and popoluate the metadata
                block[set][way].reuse[rq_entry.type]++;
                if(rq_entry.is_data && rq_entry.type == LOAD)
                    block[set][way].reuse_frontal_dorsal[ooo_cpu[read_cpu].rob_pos_get_part_type(rq_entry.rob_position)]++;
                if(knob::track_load_hit_dependency_in_cache)
                {
                    vector<uint32_t> cat_dependents;
                    cat_dependents.resize(DEP_INSTR_TYPES, 0);
                    block[set][way].dependents += ooo_cpu[read_cpu].count_dependency(rq_entry.rob_index, cat_dependents);
                    for(uint32_t i = 0; i < DEP_INSTR_TYPES; ++i) block[set][way].cat_dependents[i] = cat_dependents[i];
                    // if(warmup_complete[read_cpu] && cache_type == IS_LLC && rq_entry.type == LOAD && dependents == 0)
                    // {
                    //     ooo_cpu[read_cpu].debug_dependents(rq_entry.rob_index);
                    //     assert(0);
                    // }
                }

                // RBERA-TODO: Dump cache access trace
                if(knob::l2c_dump_access_trace && cache_type == IS_L2C && warmup_complete[read_cpu])
                {
                    tracer.record_trace(rq_entry.full_addr, rq_entry.type, true);
                }
                if(knob::llc_dump_access_trace && cache_type == IS_LLC && warmup_complete[read_cpu])
                {
                    tracer.record_trace(rq_entry.full_addr, rq_entry.type, true);
                }

                // check fill level
                if (rq_entry.fill_level < fill_level) 
                {
                    if(fill_level == FILL_L2)
                    {
                        if(rq_entry.fill_l1i)
                        {
                            upper_level_icache[read_cpu]->return_data(&rq_entry);
                        }
                        if(rq_entry.fill_l1d)
                        {
                            upper_level_dcache[read_cpu]->return_data(&rq_entry);
                        }
                    }
		            else
                    {
                        if (rq_entry.instruction)
                        {
                            upper_level_icache[read_cpu]->return_data(&rq_entry);
                        }
                        if (rq_entry.is_data)
                        {
                            upper_level_dcache[read_cpu]->return_data(&rq_entry);
                        }
                    }
                }

                // update prefetch stats and reset prefetch bit
                if (block[set][way].prefetch) 
                {
                    pf_useful++;
                    block[set][way].prefetch = 0;
                }
                block[set][way].used = 1;

                HIT[rq_entry.type]++;
                ACCESS[rq_entry.type]++;
                
                // remove this entry from RQ
                uint64_t deque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[read_cpu];
                RQ->remove_queue(&rq_entry, deque_cycle);
		        reads_available_this_cycle--;
            }
            else // read miss
            {
                DP ( if (warmup_complete[read_cpu]) {
                cout << "[" << NAME << "] " << __func__ << " read miss";
                cout << " instr_id: " << rq_entry.instr_id << " address: " << hex << rq_entry.address;
                cout << " full_addr: " << rq_entry.full_addr << dec;
                cout << " cycle: " << rq_entry.event_cycle << endl; });

                // check mshr
                uint8_t miss_handled = 1;
                int mshr_index = check_mshr(&rq_entry);

		        if(mshr_index == -2)
                {
                    // this is a data/instruction collision in the MSHR, so we have to wait before we can allocate this miss
                    miss_handled = 0;
                }
                else if ((mshr_index == -1) && (MSHR.occupancy < MSHR_SIZE)) // this is a new miss
                { 
		            if(cache_type == IS_LLC)
        		    {
		                // check to make sure the DRAM RQ has room for this LLC read miss
		                if (lower_level->get_occupancy(1, rq_entry.address) == lower_level->get_size(1, rq_entry.address))
                        {
                            miss_handled = 0;
                        }
                        else
                        {
                            rq_entry.hit_where = hit_where_t::DRAM; // LLC miss => DRAM hit
                            add_mshr(&rq_entry);
                            if(lower_level)
                            {
                                lower_level->add_rq(&rq_entry);
                                
                                // @RBERA: if this is a data load missing LLC, then:
                                // 1. Monitor the position of the load in ROB
                                // 2. Send signal to LQ and ROB about this miss
                                if(rq_entry.is_data && rq_entry.type == LOAD)
                                {
                                    if(rq_entry.rob_position < 0 || rq_entry.rob_position >= ROB_SIZE)
                                    {
                                        cout << "invalid ROB position: index: " << index << " pos: " << rq_entry.rob_position << endl;
                                        cout << rq_entry.to_string() << endl;
                                        assert(0);
                                    }
                                    missing_load_rob_pos_hist[rq_entry.rob_position]++;
                                    send_signal_to_core(read_cpu, rq_entry);
                                }
                            }
                        }
		            }
                    else
                    {
                        // add it to mshr (read miss)
                        add_mshr(&rq_entry);                    
                        // add it to the next level's read queue
                        if (lower_level)
                        {
                            if (lower_level->get_occupancy(1, rq_entry.address) == lower_level->get_size(1, rq_entry.address))
                            {
                                miss_handled = 0;
                            }
                            else
                            {
                                lower_level->add_rq(&rq_entry);
                            }
                            
                        }
                        else // this is the last level
                        {
                            if (cache_type == IS_STLB)
                            {
                                // TODO: need to differentiate page table walk and actual swap                    
                                // emulate page table walk
                                uint64_t pa = va_to_pa(read_cpu, rq_entry.instr_id, rq_entry.full_addr, rq_entry.address, 0);
                                rq_entry.data = pa >> LOG2_PAGE_SIZE; 
                                rq_entry.event_cycle = current_core_cycle[read_cpu];
                                rq_entry.hit_where = hit_where_t::PTW; // STLB miss => PTW
                                return_data(&rq_entry);
                            }
                        }
                    }
                }
                else 
                {
                    if ((mshr_index == -1) && (MSHR.occupancy == MSHR_SIZE)) // not enough MSHR resource
                    {
                        // cannot handle miss request until one of MSHRs is available
                        miss_handled = 0;
                        STALL[rq_entry.type]++;
                    }
                    else if (mshr_index != -1)  // already in-flight miss
                    {
                        rq_entry.hit_where = assign_hit_where(cache_type, 3); // MSHR hit

                        // mark merged consumer
                        if (rq_entry.type == RFO) 
                        {
                            if (rq_entry.tlb_access) 
                            {
                                uint32_t sq_index = rq_entry.sq_index;
                                MSHR.entry[mshr_index].store_merged = 1;
                                MSHR.entry[mshr_index].sq_index_depend_on_me.insert (sq_index);
				                MSHR.entry[mshr_index].sq_index_depend_on_me.join(rq_entry.sq_index_depend_on_me, SQ_SIZE);
                            }
                            if (rq_entry.load_merged) 
                            {
                                //uint32_t lq_index = rq_entry.lq_index; 
                                MSHR.entry[mshr_index].load_merged = 1;
                                //MSHR.entry[mshr_index].lq_index_depend_on_me[lq_index] = 1;
				                MSHR.entry[mshr_index].lq_index_depend_on_me.join(rq_entry.lq_index_depend_on_me, LQ_SIZE);
                            }
                        }
                        else 
                        {
                            if (rq_entry.instruction) 
                            {
                                uint32_t rob_index = rq_entry.rob_index;
                                MSHR.entry[mshr_index].instruction = 1; // add as instruction type
                                MSHR.entry[mshr_index].instr_merged = 1;
                                MSHR.entry[mshr_index].rob_index_depend_on_me.insert(rob_index);

                                DP (if (warmup_complete[MSHR.entry[mshr_index].cpu]) {
                                cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
                                cout << " merged rob_index: " << rob_index << " instr_id: " << rq_entry.instr_id << endl; });

                                if (rq_entry.instr_merged) 
                                {
				                    MSHR.entry[mshr_index].rob_index_depend_on_me.join(rq_entry.rob_index_depend_on_me, ROB_SIZE);
                                    DP (if (warmup_complete[MSHR.entry[mshr_index].cpu]) {
                                    cout << "[INSTR_MERGED] " << __func__ << " cpu: " << MSHR.entry[mshr_index].cpu << " instr_id: " << MSHR.entry[mshr_index].instr_id;
                                    cout << " merged rob_index: " << i << " instr_id: N/A" << endl; });
                                }
                            }
                            else 
                            {
                                uint32_t lq_index = rq_entry.lq_index;
                                MSHR.entry[mshr_index].is_data = 1; // add as data type
                                MSHR.entry[mshr_index].load_merged = 1;
                                MSHR.entry[mshr_index].lq_index_depend_on_me.insert(lq_index);

                                DP (if (warmup_complete[read_cpu]) {
                                cout << "[DATA_MERGED] " << __func__ << " cpu: " << read_cpu << " instr_id: " << rq_entry.instr_id;
                                cout << " merged rob_index: " << rq_entry.rob_index << " instr_id: " << rq_entry.instr_id << " lq_index: " << rq_entry.lq_index << endl; });

				                MSHR.entry[mshr_index].lq_index_depend_on_me.join(rq_entry.lq_index_depend_on_me, LQ_SIZE);
                                if (rq_entry.store_merged) 
                                {
                                    MSHR.entry[mshr_index].store_merged = 1;
				                    MSHR.entry[mshr_index].sq_index_depend_on_me.join(rq_entry.sq_index_depend_on_me, SQ_SIZE);
                                }
                            }
                        }

                        // update fill_level
                        if (rq_entry.fill_level < MSHR.entry[mshr_index].fill_level)
                        {
                            MSHR.entry[mshr_index].fill_level = rq_entry.fill_level;
                        }
			            if((rq_entry.fill_l1i) && (MSHR.entry[mshr_index].fill_l1i != 1))
                        {
                            MSHR.entry[mshr_index].fill_l1i = 1;
                        }
            			if((rq_entry.fill_l1d) && (MSHR.entry[mshr_index].fill_l1d != 1))
                        {
                            MSHR.entry[mshr_index].fill_l1d = 1;
                        }

                        // update request
                        if (MSHR.entry[mshr_index].type == PREFETCH) 
                        {
                            //RBERA: why calling it late even in case of prefetch request hitting another in-flight prefetch?
							pf_late++;
                            uint8_t  prior_returned = MSHR.entry[mshr_index].returned;
                            uint64_t prior_event_cycle = MSHR.entry[mshr_index].event_cycle;
                            MSHR.entry[mshr_index] = rq_entry;
                            
                            // in case request is already returned, we should keep event_cycle and retunred variables
                            MSHR.entry[mshr_index].returned = prior_returned;
                            MSHR.entry[mshr_index].event_cycle = prior_event_cycle;
                        }

                        MSHR_MERGED[rq_entry.type]++;

                        DP ( if (warmup_complete[read_cpu]) {
                        cout << "[" << NAME << "] " << __func__ << " mshr merged";
                        cout << " instr_id: " << rq_entry.instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id; 
                        cout << " address: " << hex << rq_entry.address;
                        cout << " full_addr: " << rq_entry.full_addr << dec;
                        cout << " cycle: " << rq_entry.event_cycle << endl; });
                    }
                    else // WE SHOULD NOT REACH HERE
                    {
                        cerr << "[" << NAME << "] MSHR errors" << endl;
                        assert(0);
                    }
                }

                if (miss_handled) 
                {
                    // update prefetcher on load instruction
		            if (rq_entry.type == LOAD) 
                    {
		                if(cache_type == IS_L1I)
                        {
			                l1i_prefetcher_cache_operate(read_cpu, rq_entry.ip, 0, 0);
                        }
                        if (cache_type == IS_L1D) 
                        {
                            l1d_prefetcher_operate(rq_entry.full_addr, rq_entry.ip, 0, rq_entry.type);
                        }
                        if (cache_type == IS_L2C)
                        {
			                l2c_prefetcher_operate(rq_entry.address<<LOG2_BLOCK_SIZE, rq_entry.ip, 0, rq_entry.type, 0);
                        }
                        if (cache_type == IS_LLC)
                        {
                            cpu = read_cpu;
                            llc_prefetcher_operate(rq_entry.address<<LOG2_BLOCK_SIZE, rq_entry.ip, 0, rq_entry.type, 0);
                            cpu = 0;
                        }
                    }

                    MISS[rq_entry.type]++;
                    ACCESS[rq_entry.type]++;

                    // remove this entry from RQ
                    uint64_t deque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[read_cpu];
                    RQ->remove_queue(&rq_entry, deque_cycle);
		            reads_available_this_cycle--;
                }
            }
        }
	    else
	    {
	        return;
	    }

	    if(reads_available_this_cycle == 0)
	    {
	        return;
	    }
    }
}

void CACHE::handle_prefetch()
{
    // handle prefetch
    for (uint32_t i=0; i<MAX_READ; i++) 
    {  
        uint32_t prefetch_cpu = PQ.entry[PQ.head].cpu;
        if (prefetch_cpu == NUM_CPUS)
        {
            return;
        }

        // handle the oldest entry
        if ((PQ.entry[PQ.head].event_cycle <= current_core_cycle[prefetch_cpu]) && (PQ.occupancy > 0)) 
        {
            int index = PQ.head;

            // access cache
            uint32_t set = get_set(PQ.entry[index].address);
            int way = check_hit(&PQ.entry[index]);
            
            if (way >= 0) // prefetch hit
            { 
                PQ.entry[index].hit_where = assign_hit_where(cache_type, 0); // prefetch hit

                // update replacement policy
                if (cache_type == IS_LLC) 
                {
                    llc_update_replacement_state(prefetch_cpu, set, way, block[set][way].full_addr, PQ.entry[index].ip, 0, PQ.entry[index].type, 1);
                }
                else
                {
                    update_replacement_state(prefetch_cpu, set, way, block[set][way].full_addr, PQ.entry[index].ip, 0, PQ.entry[index].type, 1);
                }

                // COLLECT STATS
                sim_hit[prefetch_cpu][PQ.entry[index].type]++;
                sim_access[prefetch_cpu][PQ.entry[index].type]++;

                // @RBERA: populate reuse metadata
                block[set][way].reuse[PQ.entry[index].type]++;

                // RBERA-TODO: Dump cache access trace
                if(knob::l2c_dump_access_trace && cache_type == IS_L2C && warmup_complete[prefetch_cpu])
                {
                    tracer.record_trace(PQ.entry[index].full_addr, PQ.entry[index].type, true);
                }
                if(knob::llc_dump_access_trace && cache_type == IS_LLC && warmup_complete[prefetch_cpu])
                {
                    tracer.record_trace(PQ.entry[index].full_addr, PQ.entry[index].type, true);
                }

		        // run prefetcher on prefetches from higher caches
		        if(PQ.entry[index].pf_origin_level < fill_level)
		        {
		            if (cache_type == IS_L1D)
		                l1d_prefetcher_operate(PQ.entry[index].full_addr, PQ.entry[index].ip, 1, PREFETCH);
                    else if (cache_type == IS_L2C)
                      PQ.entry[index].pf_metadata = l2c_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 1, PREFETCH, PQ.entry[index].pf_metadata);
                    else if (cache_type == IS_LLC)
    		        {
                        cpu = prefetch_cpu;
                        PQ.entry[index].pf_metadata = llc_prefetcher_operate(block[set][way].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 1, PREFETCH, PQ.entry[index].pf_metadata);
                        cpu = 0;
		            }
		        }

                // check fill level
                if (PQ.entry[index].fill_level < fill_level) 
                {
                    if(fill_level == FILL_L2)
                    {
                        if(PQ.entry[index].fill_l1i)
                        {
                            upper_level_icache[prefetch_cpu]->return_data(&PQ.entry[index]);
                        }
                        if(PQ.entry[index].fill_l1d)
                        {
                            upper_level_dcache[prefetch_cpu]->return_data(&PQ.entry[index]);
                        }
		            }
		            else
		            {
                        if (PQ.entry[index].instruction)
                        {
                            upper_level_icache[prefetch_cpu]->return_data(&PQ.entry[index]);
                        }
                        if (PQ.entry[index].is_data)
                        {
                            upper_level_dcache[prefetch_cpu]->return_data(&PQ.entry[index]);
                        }
		            }
                }

                HIT[PQ.entry[index].type]++;
                ACCESS[PQ.entry[index].type]++;
                
                // remove this entry from PQ
                uint64_t deque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[prefetch_cpu];
                PQ.remove_queue(&PQ.entry[index], deque_cycle);
		        reads_available_this_cycle--;
            }
            else // prefetch miss
            {
                DP ( if (warmup_complete[prefetch_cpu]) {
                cout << "[" << NAME << "] " << __func__ << " prefetch miss";
                cout << " instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;
                cout << " full_addr: " << PQ.entry[index].full_addr << dec << " fill_level: " << PQ.entry[index].fill_level;
                cout << " cycle: " << PQ.entry[index].event_cycle << endl; });

                // check mshr
                uint8_t miss_handled = 1;
                int mshr_index = check_mshr(&PQ.entry[index]);

		        if(mshr_index == -2)
                {
                    // this is a data/instruction collision in the MSHR, so we have to wait before we can allocate this miss
                    miss_handled = 0;
                }
                else if ((mshr_index == -1) && (MSHR.occupancy < MSHR_SIZE)) // this is a new miss
                {
                    DP ( if (warmup_complete[PQ.entry[index].cpu]) {
                    cout << "[" << NAME << "_PQ] " <<  __func__ << " want to add instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;
                    cout << " full_addr: " << PQ.entry[index].full_addr << dec;
                    cout << " occupancy: " << lower_level->get_occupancy(3, PQ.entry[index].address) << " SIZE: " << lower_level->get_size(3, PQ.entry[index].address) << endl; });

                    // first check if the lower level PQ (or RQ, in case of LLC) is full or not.
                    // this is possible since multiple prefetchers can exist at each level of caches
                    if (lower_level) 
                    {
		                if (cache_type == IS_LLC) 
                        {
			                if (lower_level->get_occupancy(1, PQ.entry[index].address) == lower_level->get_size(1, PQ.entry[index].address))
                            {
			                    miss_handled = 0;
                            }
			                else 
                            {
			                    // run prefetcher on prefetches from higher caches
			                    if(PQ.entry[index].pf_origin_level < fill_level)
                                {
                                    if (cache_type == IS_LLC)
                                    {
                                        cpu = prefetch_cpu;
                                        PQ.entry[index].pf_metadata = llc_prefetcher_operate(PQ.entry[index].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 0, PREFETCH, PQ.entry[index].pf_metadata);
                                        cpu = 0;
				                    }
			                    }

			                    // add it to MSHRs if this prefetch miss will be filled to this cache level
			                    if (PQ.entry[index].fill_level <= fill_level)
                                {
			                        add_mshr(&PQ.entry[index]);
                                }

			                    lower_level->add_rq(&PQ.entry[index]); // add it to the DRAM RQ
			                }
		                }
		                else 
                        {
			                if (lower_level->get_occupancy(3, PQ.entry[index].address) == lower_level->get_size(3, PQ.entry[index].address))
                            {
			                    miss_handled = 0;
                            }
			                else 
                            {
			                    // run prefetcher on prefetches from higher caches
			                    if(PQ.entry[index].pf_origin_level < fill_level)
                                {
                                    if (cache_type == IS_L1D)
                                    {
                                        l1d_prefetcher_operate(PQ.entry[index].full_addr, PQ.entry[index].ip, 0, PREFETCH);
                                    }
                                    if (cache_type == IS_L2C)
                                    {
                                        PQ.entry[index].pf_metadata = l2c_prefetcher_operate(PQ.entry[index].address<<LOG2_BLOCK_SIZE, PQ.entry[index].ip, 0, PREFETCH, PQ.entry[index].pf_metadata);
                                    }
                                }
			  
			                    // add it to MSHRs if this prefetch miss will be filled to this cache level
			                    if (PQ.entry[index].fill_level <= fill_level)
                                {
			                        add_mshr(&PQ.entry[index]);
                                }

			                    lower_level->add_pq(&PQ.entry[index]); // add it to the PQ of lower cache
			                }
		                }
		            }
                }
                else 
                {
                    if ((mshr_index == -1) && (MSHR.occupancy == MSHR_SIZE)) // not enough MSHR resource
                    { 
                        // TODO: should we allow prefetching with lower fill level at this case?
                        
                        // cannot handle miss request until one of MSHRs is available
                        miss_handled = 0;
                        STALL[PQ.entry[index].type]++;
                    }
                    else if (mshr_index != -1)  // already in-flight miss
                    {
                        PQ.entry[index].hit_where = assign_hit_where(cache_type, 3); // prefetch hit in MSHR

                        // no need to update request except fill_level
                        // update fill_level
                        if (PQ.entry[index].fill_level < MSHR.entry[mshr_index].fill_level)
                        {
                            MSHR.entry[mshr_index].fill_level = PQ.entry[index].fill_level;
                        }

                        if((PQ.entry[index].fill_l1i) && (MSHR.entry[mshr_index].fill_l1i != 1))
                        {
                            MSHR.entry[mshr_index].fill_l1i = 1;
                        }
                        if((PQ.entry[index].fill_l1d) && (MSHR.entry[mshr_index].fill_l1d != 1))
                        {
                            MSHR.entry[mshr_index].fill_l1d = 1;
                        }

                        MSHR_MERGED[PQ.entry[index].type]++;

                        DP ( if (warmup_complete[prefetch_cpu]) {
                        cout << "[" << NAME << "] " << __func__ << " mshr merged";
                        cout << " instr_id: " << PQ.entry[index].instr_id << " prior_id: " << MSHR.entry[mshr_index].instr_id; 
                        cout << " address: " << hex << PQ.entry[index].address;
                        cout << " full_addr: " << PQ.entry[index].full_addr << dec << " fill_level: " << MSHR.entry[mshr_index].fill_level;
                        cout << " cycle: " << MSHR.entry[mshr_index].event_cycle << endl; });
                    }
                    else // WE SHOULD NOT REACH HERE
                    { 
                        cerr << "[" << NAME << "] MSHR errors" << endl;
                        assert(0);
                    }
                }

                if (miss_handled) 
                {
                    DP ( if (warmup_complete[prefetch_cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " prefetch miss handled";
                    cout << " instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;
                    cout << " full_addr: " << PQ.entry[index].full_addr << dec << " fill_level: " << PQ.entry[index].fill_level;
                    cout << " cycle: " << PQ.entry[index].event_cycle << endl; });

                    MISS[PQ.entry[index].type]++;
                    ACCESS[PQ.entry[index].type]++;

                    // remove this entry from PQ
                    uint64_t deque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[prefetch_cpu];
                    PQ.remove_queue(&PQ.entry[index], deque_cycle);
		            reads_available_this_cycle--;
                }
            }
        }
	    else
	    {
	        return;
	    }

	    if(reads_available_this_cycle == 0)
        {
            return;
        }
    }
}

void CACHE::operate()
{
    handle_fill();
    handle_writeback();
    reads_available_this_cycle = MAX_READ;
    handle_read();

    if (PQ.occupancy && (reads_available_this_cycle > 0))
        handle_prefetch();
}

uint32_t CACHE::get_set(uint64_t address)
{
    return (uint32_t) (address & ((1 << lg2(NUM_SET)) - 1)); 
}

uint32_t CACHE::get_way(uint64_t address, uint32_t set)
{
    for (uint32_t way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid && (block[set][way].tag == address)) 
            return way;
    }

    return NUM_WAY;
}

void CACHE::fill_cache(uint32_t set, uint32_t way, PACKET *packet)
{
#ifdef SANITY_CHECK
    if (cache_type == IS_ITLB) {
        if (packet->data == 0)
            assert(0);
    }

    if (cache_type == IS_DTLB) {
        if (packet->data == 0)
            assert(0);
    }

    if (cache_type == IS_STLB) {
        if (packet->data == 0)
            assert(0);
    }
#endif
    if (block[set][way].prefetch && (block[set][way].used == 0))
        pf_useless++;
    
    if(block[set][way].valid == 1) // eviction
    {
        /* call any routine before eviction */
        track_stats_from_victim(set, way);
    }

    if (block[set][way].valid == 0)
        block[set][way].valid = 1;
    block[set][way].dirty = 0;
    block[set][way].prefetch = (packet->type == PREFETCH) ? 1 : 0;
    block[set][way].used = 0;

    if (block[set][way].prefetch)
    {
        pf_filled++;
    }

    block[set][way].delta = packet->delta;
    block[set][way].depth = packet->depth;
    block[set][way].signature = packet->signature;
    block[set][way].confidence = packet->confidence;

    block[set][way].tag = packet->address;
    block[set][way].address = packet->address;
    block[set][way].full_addr = packet->full_addr;
    block[set][way].data = packet->data;
    block[set][way].ip = packet->ip;
    block[set][way].cpu = packet->cpu;
    block[set][way].instr_id = packet->instr_id;

    block[set][way].reset_metadata();
    block[set][way].fill_ip = packet->ip;

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "] " << __func__ << " set: " << set << " way: " << way;
    cout << " lru: " << block[set][way].lru << " tag: " << hex << block[set][way].tag << " full_addr: " << block[set][way].full_addr;
    cout << " data: " << block[set][way].data << dec << endl; });
}

int CACHE::check_hit(PACKET *packet)
{
    uint32_t set = get_set(packet->address);
    int match_way = -1;

    if (NUM_SET < set) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << NUM_SET;
        cerr << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
        cerr << " event: " << packet->event_cycle << endl;
        assert(0);
    }

    // perfect cache
    if((cache_type == IS_L1D && knob::l1d_perfect)
        || (cache_type == IS_L2C && knob::l2c_perfect)
        || (cache_type == IS_LLC && knob::llc_perfect))
    {
        match_way = 0;
        return match_way;
    }

    // hit
    for (uint32_t way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid && (block[set][way].tag == packet->address)) {

            match_way = way;

            DP ( if (warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "] " << __func__ << " instr_id: " << packet->instr_id << " type: " << +packet->type << hex << " addr: " << packet->address;
            cout << " full_addr: " << packet->full_addr << " tag: " << block[set][way].tag << " data: " << block[set][way].data << dec;
            cout << " set: " << set << " way: " << way << " lru: " << block[set][way].lru;
            cout << " event: " << packet->event_cycle << " cycle: " << current_core_cycle[cpu] << endl; });

            break;
        }
    }
    
    // pseudo-perfect LLC for missing frontal/dorsal loads
    if(packet->is_data && packet->type == LOAD    // this should only be modeled for data loads
        && match_way == -1                        // that are missing in the actual cache
        && (cache_type == IS_LLC && knob::llc_pseudo_perfect_enable))
    {
        stats.pseudo_perfect.data_load_misses++;

        if((knob::llc_pseudo_perfect_enable_frontal && ooo_cpu[packet->cpu].rob_pos_is_frontal(packet->rob_position))
            || (knob::llc_pseudo_perfect_enable_dorsal && ooo_cpu[packet->cpu].rob_pos_is_dorsal(packet->rob_position)))
        {
            stats.pseudo_perfect.data_load_miss_eligible_for_pseudo_hit_promotion++;
            
            // promote to hit with a given probability
            if((*dist)(generator))
            {
                stats.pseudo_perfect.data_load_miss_promoted_pseudo_hit++;
                match_way = 0;
            }
        }
    }

    // pseudo-perfect L2C for missing frontal/dorsal loads
    if(packet->is_data && packet->type == LOAD    // this should only be modeled for data loads
        && match_way == -1                        // that are missing in the actual cache
        && (cache_type == IS_L2C && knob::l2c_pseudo_perfect_enable))
    {
        stats.pseudo_perfect.data_load_misses++;

        if((knob::l2c_pseudo_perfect_enable_frontal && ooo_cpu[packet->cpu].rob_pos_is_frontal(packet->rob_position))
            || (knob::l2c_pseudo_perfect_enable_dorsal && ooo_cpu[packet->cpu].rob_pos_is_dorsal(packet->rob_position)))
        {
            stats.pseudo_perfect.data_load_miss_eligible_for_pseudo_hit_promotion++;
            
            // promote to hit with a given probability
            if((*dist)(generator))
            {
                stats.pseudo_perfect.data_load_miss_promoted_pseudo_hit++;
                match_way = 0;
            }
        }
    }

    return match_way;
}

int CACHE::invalidate_entry(uint64_t inval_addr)
{
    uint32_t set = get_set(inval_addr);
    int match_way = -1;

    if (NUM_SET < set) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << NUM_SET;
        cerr << " inval_addr: " << hex << inval_addr << dec << endl;
        assert(0);
    }

    // invalidate
    for (uint32_t way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid && (block[set][way].tag == inval_addr)) {

            block[set][way].valid = 0;

            match_way = way;

            DP ( if (warmup_complete[cpu]) {
            cout << "[" << NAME << "] " << __func__ << " inval_addr: " << hex << inval_addr;  
            cout << " tag: " << block[set][way].tag << " data: " << block[set][way].data << dec;
            cout << " set: " << set << " way: " << way << " lru: " << block[set][way].lru << " cycle: " << current_core_cycle[cpu] << endl; });

            break;
        }
    }

    return match_way;
}

int CACHE::add_rq(PACKET *packet)
{
    // check for the latest writebacks in the write queue
    int wq_index = WQ.check_queue(packet);
    if (wq_index != -1) 
    {
        packet->hit_where = assign_hit_where(cache_type, 2); // hit in WQ

        // check fill level
        if (packet->fill_level < fill_level) 
        {
            packet->data = WQ.entry[wq_index].data;

	        if(fill_level == FILL_L2)
	        {
		        if(packet->fill_l1i)
		        {
		            upper_level_icache[packet->cpu]->return_data(packet);
		        }
		        if(packet->fill_l1d)
		        {
		            upper_level_dcache[packet->cpu]->return_data(packet);
		        }
	        }
	        else
	        {
		        if (packet->instruction)
                {
		            upper_level_icache[packet->cpu]->return_data(packet);
                }
		        if (packet->is_data)
                {
		            upper_level_dcache[packet->cpu]->return_data(packet);
                }
	        }
        }

#ifdef SANITY_CHECK
        if (cache_type == IS_ITLB)
            assert(0);
        else if (cache_type == IS_DTLB)
            assert(0);
        else if (cache_type == IS_L1I)
            assert(0);
#endif
        // update processed packets
        if ((cache_type == IS_L1D) && (packet->type != PREFETCH)) 
        {
            if (PROCESSED.occupancy < PROCESSED.SIZE)
                PROCESSED.add_queue(packet, current_core_cycle[packet->cpu]);

            DP ( if (warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "_RQ] " << __func__ << " instr_id: " << packet->instr_id << " found recent writebacks";
            cout << hex << " read: " << packet->address << " writeback: " << WQ.entry[wq_index].address << dec;
            cout << " index: " << MAX_READ << " rob_signal: " << packet->rob_signal << endl; });
        }

        HIT[packet->type]++;
        ACCESS[packet->type]++;

        WQ.FORWARD++;
        RQ->ACCESS++;

        return -1;
    }

    // check for duplicates in the read queue
    int index = RQ->check_queue(packet);
    if (index != -1) 
    {
        packet->hit_where = assign_hit_where(cache_type, 1); // hit in RQ
        PACKET& rq_entry = RQ->get_entry(index);

        if (packet->instruction) 
        {
            uint32_t rob_index = packet->rob_index;
            rq_entry.rob_index_depend_on_me.insert(rob_index);
            rq_entry.instruction = 1; // add as instruction type
            rq_entry.instr_merged = 1;

            DP (if (warmup_complete[packet->cpu]) {
            cout << "[INSTR_MERGED] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << rq_entry.instr_id;
            cout << " merged rob_index: " << rob_index << " instr_id: " << packet->instr_id << endl; });
        }
        else 
        {
            // mark merged consumer
            if (packet->type == RFO) 
            {
                uint32_t sq_index = packet->sq_index;
                rq_entry.sq_index_depend_on_me.insert(sq_index);
                rq_entry.store_merged = 1;
                
                DP (if (warmup_complete[packet->cpu]) {
                cout << "[RFO merging in RQ] cache_type " << cache_type
                << " incoming_type " << packet->type << " exsisting_type " << rq_entry.type
                << " incoming_ROB_index " << packet->rob_index << " exsisting_ROB_index " << rq_entry.rob_index
                << " incoming_ROB_pos " << packet->rob_position << " exsisting_ROB_pos " << rq_entry.rob_position << endl; });
            }
            else 
            {
                uint32_t lq_index = packet->lq_index; 
                rq_entry.lq_index_depend_on_me.insert(lq_index);
                rq_entry.load_merged = 1;

                DP (if (warmup_complete[packet->cpu]) {
                cout << "[LOAD merging in RQ] cache_type " << cache_type
                << " incoming_type " << packet->type << " exsisting_type " << rq_entry.type
                << " incoming_ROB_index " << packet->rob_index << " exsisting_ROB_index " << rq_entry.rob_index
                << " incoming_ROB_pos " << packet->rob_position << " exsisting_ROB_pos " << rq_entry.rob_position << endl; });

                DP (if (warmup_complete[packet->cpu]) {
                cout << "[DATA_MERGED] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << rq_entry.instr_id;
                cout << " merged rob_index: " << packet->rob_index << " instr_id: " << packet->instr_id << " lq_index: " << packet->lq_index << endl; });
            }
            rq_entry.is_data = 1; // add as data type
        }

	    if((packet->fill_l1i) && (rq_entry.fill_l1i != 1))
	    {
	        rq_entry.fill_l1i = 1;
	    }
	    if((packet->fill_l1d) && (rq_entry.fill_l1d != 1))
	    {
	        rq_entry.fill_l1d = 1;
	    }

        RQ->MERGED++;
        RQ->ACCESS++;

        return index; // merged index
    }

    // check occupancy
    if (RQ->occupancy == RQ_SIZE) 
    {
        RQ->FULL++;
        return -2; // cannot handle this request
    }

    // if there is no duplicate, add it to RQ
    uint64_t enque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[packet->cpu];
    index = RQ->add_queue(packet, enque_cycle);
    PACKET& rq_entry = RQ->get_entry(index);

    // ADD LATENCY
    if (rq_entry.event_cycle < current_core_cycle[packet->cpu])
    {
        rq_entry.event_cycle = current_core_cycle[packet->cpu] + LATENCY;
    }
    else
    {
        rq_entry.event_cycle += LATENCY;
    }

    DP ( if (warmup_complete[rq_entry.cpu]) {
    cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << rq_entry.instr_id << " address: " << hex << rq_entry.address;
    cout << " full_addr: " << rq_entry.full_addr << dec;
    cout << " type: " << +rq_entry.type << " head: " << RQ->get_head() << " tail: " << RQ->get_tail() << " occupancy: " << RQ->occupancy;
    cout << " event: " << rq_entry.event_cycle << " current: " << current_core_cycle[rq_entry.cpu] << endl; });

    if (packet->address == 0)
        assert(0);

    RQ->TO_CACHE++;
    RQ->ACCESS++;

    return -1;
}

int CACHE::add_wq(PACKET *packet)
{
    // check for duplicates in the write queue
    int index = WQ.check_queue(packet);
    if (index != -1) 
    {

        WQ.MERGED++;
        WQ.ACCESS++;

        return index; // merged index
    }

    // sanity check
    if (WQ.occupancy >= WQ.SIZE)
        assert(0);

    // if there is no duplicate, add it to the write queue
    uint64_t enque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[packet->cpu];
    index = WQ.add_queue(packet, enque_cycle);
    // index = WQ.tail;
    // if (WQ.entry[index].address != 0) 
    // {
    //     cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
    //     cerr << " address: " << hex << WQ.entry[index].address;
    //     cerr << " full_addr: " << WQ.entry[index].full_addr << dec << endl;
    //     assert(0);
    // }

    // WQ.entry[index] = *packet;

    // ADD LATENCY
    if (WQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
    {
        WQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
    }
    else
    {
        WQ.entry[index].event_cycle += LATENCY;
    }

    // WQ.occupancy++;
    // WQ.tail++;
    // if (WQ.tail >= WQ.SIZE)
    //     WQ.tail = 0;

    DP (if (warmup_complete[WQ.entry[index].cpu]) {
    cout << "[" << NAME << "_WQ] " <<  __func__ << " instr_id: " << WQ.entry[index].instr_id << " address: " << hex << WQ.entry[index].address;
    cout << " full_addr: " << WQ.entry[index].full_addr << dec;
    cout << " head: " << WQ.head << " tail: " << WQ.tail << " occupancy: " << WQ.occupancy;
    cout << " data: " << hex << WQ.entry[index].data << dec;
    cout << " event: " << WQ.entry[index].event_cycle << " current: " << current_core_cycle[WQ.entry[index].cpu] << endl; });

    WQ.TO_CACHE++;
    WQ.ACCESS++;

    return -1;
}

int CACHE::prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int pf_fill_level, uint32_t prefetch_metadata)
{
    pf_requested++;

    if (PQ.occupancy < PQ.SIZE) 
    {
        if ((base_addr>>LOG2_PAGE_SIZE) == (pf_addr>>LOG2_PAGE_SIZE)) 
        {
            PACKET pf_packet;
            pf_packet.fill_level = pf_fill_level;
	        pf_packet.pf_origin_level = fill_level;
            if(pf_fill_level == FILL_L1)
            {
                pf_packet.fill_l1d = 1;
            }
	        pf_packet.pf_metadata = prefetch_metadata;
            pf_packet.cpu = cpu;
            //pf_packet.data_index = LQ.entry[lq_index].data_index;
            //pf_packet.lq_index = lq_index;
            pf_packet.address = pf_addr >> LOG2_BLOCK_SIZE;
            pf_packet.full_addr = pf_addr;
            //pf_packet.instr_id = LQ.entry[lq_index].instr_id;
            //pf_packet.rob_index = LQ.entry[lq_index].rob_index;
            pf_packet.ip = ip;
            pf_packet.type = PREFETCH;
            pf_packet.event_cycle = current_core_cycle[cpu];

            // give a dummy 0 as the IP of a prefetch
            add_pq(&pf_packet);

            pf_issued++;

            return 1;
        }
    }
    else
    {
        pf_dropped++;
    }

    return 0;
}

int CACHE::kpc_prefetch_line(uint64_t base_addr, uint64_t pf_addr, int pf_fill_level, int delta, int depth, int signature, int confidence, uint32_t prefetch_metadata)
{
    if (PQ.occupancy < PQ.SIZE) 
    {
        if ((base_addr>>LOG2_PAGE_SIZE) == (pf_addr>>LOG2_PAGE_SIZE)) 
        {
            PACKET pf_packet;
            pf_packet.fill_level = pf_fill_level;
	        pf_packet.pf_origin_level = fill_level;
            if(pf_fill_level == FILL_L1)
            {
                pf_packet.fill_l1d = 1;
            }
	        pf_packet.pf_metadata = prefetch_metadata;
            pf_packet.cpu = cpu;
            //pf_packet.data_index = LQ.entry[lq_index].data_index;
            //pf_packet.lq_index = lq_index;
            pf_packet.address = pf_addr >> LOG2_BLOCK_SIZE;
            pf_packet.full_addr = pf_addr;
            //pf_packet.instr_id = LQ.entry[lq_index].instr_id;
            //pf_packet.rob_index = LQ.entry[lq_index].rob_index;
            pf_packet.ip = 0;
            pf_packet.type = PREFETCH;
            pf_packet.delta = delta;
            pf_packet.depth = depth;
            pf_packet.signature = signature;
            pf_packet.confidence = confidence;
            pf_packet.event_cycle = current_core_cycle[cpu];

            // give a dummy 0 as the IP of a prefetch
            add_pq(&pf_packet);

            pf_issued++;

            return 1;
        }
    }

    return 0;
}

int CACHE::add_pq(PACKET *packet)
{
    // check for the latest wirtebacks in the write queue
    int wq_index = WQ.check_queue(packet);
    if (wq_index != -1) 
    {
        packet->hit_where = assign_hit_where(cache_type, 2); // prefetch hitting in WQ

        // check fill level
        if (packet->fill_level < fill_level) 
        {
            packet->data = WQ.entry[wq_index].data;

	        if(fill_level == FILL_L2)
	        {
                if(packet->fill_l1i)
                {
                    upper_level_icache[packet->cpu]->return_data(packet);
                }
                if(packet->fill_l1d)
		        {
		            upper_level_dcache[packet->cpu]->return_data(packet);
		        }
	        }
	        else
	        {
                if (packet->instruction)
                {
                    upper_level_icache[packet->cpu]->return_data(packet);
                }
                if (packet->is_data)
                {
                    upper_level_dcache[packet->cpu]->return_data(packet);
                }
	        }
        }

        HIT[packet->type]++;
        ACCESS[packet->type]++;

        WQ.FORWARD++;
        PQ.ACCESS++;

        return -1;
    }

    // check for duplicates in the PQ
    int index = PQ.check_queue(packet);
    if (index != -1) 
    {
        if (packet->fill_level < PQ.entry[index].fill_level)
	    {
            PQ.entry[index].fill_level = packet->fill_level;
	    }
	    if((packet->instruction == 1) && (PQ.entry[index].instruction != 1))
	    {
	        PQ.entry[index].instruction = 1;
	    }
	    if((packet->is_data == 1) && (PQ.entry[index].is_data != 1))
	    {
	        PQ.entry[index].is_data = 1;
	    }
	    if((packet->fill_l1i) && (PQ.entry[index].fill_l1i != 1))
	    {
	        PQ.entry[index].fill_l1i = 1;
	    }
	    if((packet->fill_l1d) && (PQ.entry[index].fill_l1d != 1))
	    {
	        PQ.entry[index].fill_l1d = 1;
	    }

        PQ.MERGED++;
        PQ.ACCESS++;

        return index; // merged index
    }

    // check occupancy
    if (PQ.occupancy == PQ_SIZE) 
    {
        PQ.FULL++;

        DP ( if (warmup_complete[packet->cpu]) {
        cout << "[" << NAME << "] cannot process add_pq since it is full" << endl; });
        return -2; // cannot handle this request
    }

    // if there is no duplicate, add it to PQ
    uint64_t enque_cycle = cache_type == IS_LLC ? uncore.cycle : current_core_cycle[packet->cpu];
    index = PQ.add_queue(packet, enque_cycle);
//     index = PQ.tail;

// #ifdef SANITY_CHECK
//     if (PQ.entry[index].address != 0) {
//         cerr << "[" << NAME << "_ERROR] " << __func__ << " is not empty index: " << index;
//         cerr << " address: " << hex << PQ.entry[index].address;
//         cerr << " full_addr: " << PQ.entry[index].full_addr << dec << endl;
//         assert(0);
//     }
// #endif

//     PQ.entry[index] = *packet;

    // ADD LATENCY
    if (PQ.entry[index].event_cycle < current_core_cycle[packet->cpu])
    {
        PQ.entry[index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
    }
    else
    {
        PQ.entry[index].event_cycle += LATENCY;
    }

    // PQ.occupancy++;
    // PQ.tail++;
    // if (PQ.tail >= PQ.SIZE)
    //     PQ.tail = 0;

    DP ( if (warmup_complete[PQ.entry[index].cpu]) {
    cout << "[" << NAME << "_PQ] " <<  __func__ << " instr_id: " << PQ.entry[index].instr_id << " address: " << hex << PQ.entry[index].address;
    cout << " full_addr: " << PQ.entry[index].full_addr << dec;
    cout << " type: " << +PQ.entry[index].type << " head: " << PQ.head << " tail: " << PQ.tail << " occupancy: " << PQ.occupancy;
    cout << " event: " << PQ.entry[index].event_cycle << " current: " << current_core_cycle[PQ.entry[index].cpu] << endl; });

    if (packet->address == 0)
        assert(0);

    PQ.TO_CACHE++;
    PQ.ACCESS++;

    return -1;
}

void CACHE::return_data(PACKET *packet)
{
    // check MSHR information
    int mshr_index = check_mshr(packet);

    // sanity check
    if (mshr_index == -1) {
        cerr << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id << " cannot find a matching entry!";
        cerr << " full_addr: " << hex << packet->full_addr;
        cerr << " address: " << packet->address << dec;
        cerr << " event: " << packet->event_cycle << " current: " << current_core_cycle[packet->cpu] << endl;
        assert(0);
    }

    // MSHR holds the most updated information about this request
    // no need to do memcpy
    MSHR.num_returned++;
    MSHR.entry[mshr_index].returned = COMPLETED;
    MSHR.entry[mshr_index].data = packet->data;
    MSHR.entry[mshr_index].pf_metadata = packet->pf_metadata;

    // ADD LATENCY
    if (MSHR.entry[mshr_index].event_cycle < current_core_cycle[packet->cpu])
        MSHR.entry[mshr_index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
    else
        MSHR.entry[mshr_index].event_cycle += LATENCY;

    update_fill_cycle();

    DP (if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "_MSHR] " <<  __func__ << " instr_id: " << MSHR.entry[mshr_index].instr_id;
    cout << " address: " << hex << MSHR.entry[mshr_index].address << " full_addr: " << MSHR.entry[mshr_index].full_addr;
    cout << " data: " << MSHR.entry[mshr_index].data << dec << " num_returned: " << MSHR.num_returned;
    cout << " index: " << mshr_index << " occupancy: " << MSHR.occupancy;
    cout << " event: " << MSHR.entry[mshr_index].event_cycle << " current: " << current_core_cycle[packet->cpu] << " next: " << MSHR.next_fill_cycle << endl; });
}

void CACHE::update_fill_cycle()
{
    // update next_fill_cycle
    uint64_t min_cycle = UINT64_MAX;
    uint32_t min_index = MSHR.SIZE;
    for (uint32_t i=0; i<MSHR.SIZE; i++) {
        if ((MSHR.entry[i].returned == COMPLETED) && (MSHR.entry[i].event_cycle < min_cycle)) {
            min_cycle = MSHR.entry[i].event_cycle;
            min_index = i;
        }

        DP (if (warmup_complete[MSHR.entry[i].cpu]) {
        cout << "[" << NAME << "_MSHR] " <<  __func__ << " checking instr_id: " << MSHR.entry[i].instr_id;
        cout << " address: " << hex << MSHR.entry[i].address << " full_addr: " << MSHR.entry[i].full_addr;
        cout << " data: " << MSHR.entry[i].data << dec << " returned: " << +MSHR.entry[i].returned << " fill_level: " << MSHR.entry[i].fill_level;
        cout << " index: " << i << " occupancy: " << MSHR.occupancy;
        cout << " event: " << MSHR.entry[i].event_cycle << " current: " << current_core_cycle[MSHR.entry[i].cpu] << " next: " << MSHR.next_fill_cycle << endl; });
    }
    
    MSHR.next_fill_cycle = min_cycle;
    MSHR.next_fill_index = min_index;
    if (min_index < MSHR.SIZE) {

        DP (if (warmup_complete[MSHR.entry[min_index].cpu]) {
        cout << "[" << NAME << "_MSHR] " <<  __func__ << " instr_id: " << MSHR.entry[min_index].instr_id;
        cout << " address: " << hex << MSHR.entry[min_index].address << " full_addr: " << MSHR.entry[min_index].full_addr;
        cout << " data: " << MSHR.entry[min_index].data << dec << " num_returned: " << MSHR.num_returned;
        cout << " event: " << MSHR.entry[min_index].event_cycle << " current: " << current_core_cycle[MSHR.entry[min_index].cpu] << " next: " << MSHR.next_fill_cycle << endl; });
    }
}

int CACHE::check_mshr(PACKET *packet)
{
    // search mshr
  //bool instruction_and_data_collision = false;
  
    for (uint32_t index=0; index<MSHR_SIZE; index++)
    {
        if (MSHR.entry[index].address == packet->address)
	    {
            //if(MSHR.entry[index].instruction != packet->instruction)
            //  {
            //    instruction_and_data_collision = true;
            //  }
            //else
            //  {
            DP ( if (warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "_MSHR] " << __func__ << " same entry instr_id: " << packet->instr_id << " prior_id: " << MSHR.entry[index].instr_id;
            cout << " address: " << hex << packet->address;
            cout << " full_addr: " << packet->full_addr << dec << endl; });
	    
	        return index;
	        //  }
	    }
    }

    //if(instruction_and_data_collision) // remove instruction-and-data collision safeguard
    //  {
	//return -2;
    //  }

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "_MSHR] " << __func__ << " new address: " << hex << packet->address;
    cout << " full_addr: " << packet->full_addr << dec << endl; });

    DP ( if (warmup_complete[packet->cpu] && (MSHR.occupancy == MSHR_SIZE)) { 
    cout << "[" << NAME << "_MSHR] " << __func__ << " mshr is full";
    cout << " instr_id: " << packet->instr_id << " mshr occupancy: " << MSHR.occupancy;
    cout << " address: " << hex << packet->address;
    cout << " full_addr: " << packet->full_addr << dec;
    cout << " cycle: " << current_core_cycle[packet->cpu] << endl; });

    return -1;
}

void CACHE::add_mshr(PACKET *packet)
{
    uint32_t index = 0;

    packet->cycle_enqueued = current_core_cycle[packet->cpu];

    // search mshr
    for (index=0; index<MSHR_SIZE; index++) 
    {
        if (MSHR.entry[index].address == 0) 
        {
            MSHR.entry[index] = *packet;
            MSHR.entry[index].returned = INFLIGHT;
            MSHR.occupancy++;

            DP ( if (warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id;
            cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
            cout << " index: " << index << " occupancy: " << MSHR.occupancy << endl; });

            break;
        }
    }
}

uint32_t CACHE::get_occupancy(uint8_t queue_type, uint64_t address)
{
    if (queue_type == 0)
        return MSHR.occupancy;
    else if (queue_type == 1)
        return RQ->occupancy;
    else if (queue_type == 2)
        return WQ.occupancy;
    else if (queue_type == 3)
        return PQ.occupancy;

    return 0;
}

uint32_t CACHE::get_size(uint8_t queue_type, uint64_t address)
{
    if (queue_type == 0)
        return MSHR.SIZE;
    else if (queue_type == 1)
        return RQ->SIZE;
    else if (queue_type == 2)
        return WQ.SIZE;
    else if (queue_type == 3)
        return PQ.SIZE;

    return 0;
}

void CACHE::increment_WQ_FULL(uint64_t address)
{
    WQ.FULL++;
}

void CACHE::prefetcher_feedback(uint64_t &pref_gen, uint64_t &pref_fill, uint64_t &pref_used, uint64_t &pref_late)
{
    pref_gen = pf_issued;
    pref_fill = pf_filled;
    pref_used = pf_useful;
    pref_late = pf_late;
}

void CACHE::track_stats_from_victim(uint32_t set, uint32_t way)
{
    stats.eviction.total++;

    // keep count of blocks that have seen at least one reuse of specific access type
    for(uint32_t type = LOAD; type < NUM_TYPES; ++type)
    {
        if(block[set][way].reuse[type] > 0) 
        {
            stats.eviction.atleast_one_reuse++;
            stats.eviction.atleast_one_reuse_cat[type]++;
        }
    }

    // track total reuse
    uint32_t reuse = 0;
    for(uint32_t index = 0; index < NUM_TYPES; ++index) reuse += block[set][way].reuse[index];
    stats.eviction.all_reuse_total += reuse;
    if(reuse >= stats.eviction.all_reuse_max) stats.eviction.all_reuse_max = reuse;
    if(reuse <= stats.eviction.all_reuse_min) stats.eviction.all_reuse_min = reuse;

    // track reuse of individual access type
    for(uint32_t type = LOAD; type < NUM_TYPES; ++type)
    {
        reuse = block[set][way].reuse[type];
        stats.eviction.cat_reuse_total[type] += reuse;
        if(reuse >= stats.eviction.cat_reuse_max[type]) stats.eviction.cat_reuse_max[type] = reuse;
        if(reuse <= stats.eviction.cat_reuse_min[type]) stats.eviction.cat_reuse_min[type] = reuse;
    }

    // track cacheblocks that are reused by only frontal/dorsal loads
    if(block[set][way].reuse_frontal_dorsal[FRONTAL] > 0 
        && block[set][way].reuse_frontal_dorsal[DORSAL] == 0 
        && block[set][way].reuse_frontal_dorsal[NONE] == 0)
    {
        stats.eviction.reuse_only_frontal++;
    }
    else if(block[set][way].reuse_frontal_dorsal[DORSAL] > 0 
        && block[set][way].reuse_frontal_dorsal[FRONTAL] == 0 
        && block[set][way].reuse_frontal_dorsal[NONE] == 0)
    {
        stats.eviction.reuse_only_dorsal++;
    }
    else if(block[set][way].reuse_frontal_dorsal[NONE] > 0 
        && block[set][way].reuse_frontal_dorsal[FRONTAL] == 0 
        && block[set][way].reuse_frontal_dorsal[DORSAL] == 0)
    {
        stats.eviction.reuse_only_none++;
    }
    else if(block[set][way].reuse[LOAD] > 0)
    {
        stats.eviction.reuse_mixed++; // this still counts instruction load reuses
    }

    // track number of dependents for only those blocks that have seen at least one load reuse
    if(knob::track_load_hit_dependency_in_cache && block[set][way].reuse[LOAD] > 0)
    {
        // total dependents
        uint32_t dep_all = block[set][way].dependents;
        stats.eviction.dep_all_total += dep_all;
        if(dep_all >= stats.eviction.dep_all_max) stats.eviction.dep_all_max = dep_all;
        if(dep_all <= stats.eviction.dep_all_min) stats.eviction.dep_all_min = dep_all;
        if(dependent_map.find(dep_all) == dependent_map.end())
            dependent_map.insert(std::pair<uint64_t, uint64_t>(dep_all, 1));
        else
            dependent_map[dep_all]++;

        // mispredicted branches in the dependency chain
        uint32_t dep_branch_mispred = block[set][way].cat_dependents[DEP_INSTR_BRANCH_MISPRED];
        stats.eviction.dep_branch_mispred_total += dep_branch_mispred;
        if(dep_branch_mispred >= stats.eviction.dep_branch_mispred_max) stats.eviction.dep_branch_mispred_max = dep_branch_mispred;
        if(dep_branch_mispred <= stats.eviction.dep_branch_mispred_min) stats.eviction.dep_branch_mispred_min = dep_branch_mispred;

        // all branches (correct+mispred) in the dependency chain
        uint32_t dep_branch = block[set][way].cat_dependents[DEP_INSTR_BRANCH_MISPRED] + block[set][way].cat_dependents[DEP_INSTR_BRANCH_CORRECT];
        stats.eviction.dep_branch_total += dep_branch;
        if(dep_branch >= stats.eviction.dep_branch_max) stats.eviction.dep_branch_max = dep_branch;
        if(dep_branch <= stats.eviction.dep_branch_min) stats.eviction.dep_branch_min = dep_branch;

        // loads in the dependency chain
        uint32_t dep_load = block[set][way].cat_dependents[DEP_INSTR_LOAD];
        stats.eviction.dep_load_total += dep_load;
        if(dep_load >= stats.eviction.dep_load_max) stats.eviction.dep_load_max = dep_load;
        if(dep_load <= stats.eviction.dep_load_min) stats.eviction.dep_load_min = dep_load;
    }
}

/* RBERA: this function is not fully complete.
 * Meaning, this does not faithfully assign hit_where for all cases.
 * PLEASE DO NOT USE THIS AS IS.
 */
hit_where_t CACHE::assign_hit_where(uint8_t cache_type, uint32_t where_in_cache)
{
    // where_in_cache:
    // 0 = hit in cache data array
    // 1 = hit in read queue (possible for L1D/L1I. Basically caches that are core-faced)
    // 2 = hit in WQ (possible for L1D/L1I).
    // 3 = hit in MSHR

    if(cache_type == IS_ITLB)
    {
        // TODO: can ITLB have RQ/WQ hits too?
        if(where_in_cache == 0)         return hit_where_t::ITLB;
        else if(where_in_cache == 3)    return hit_where_t::ITLB_MSHR;
    }
    else if(cache_type == IS_DTLB)
    {
        // TODO: can DTLB have RQ/WQ hits too?
        if(where_in_cache == 0)         return hit_where_t::DTLB;
        else if(where_in_cache == 3)    return hit_where_t::DTLB;
    }
    else if(cache_type == STLB)
    {
        return hit_where_t::STLB;
    }
    else if(cache_type == IS_L1I)
    {
        if(where_in_cache == 0)         return hit_where_t::L1I;
        else if(where_in_cache == 1)    return hit_where_t::L1I_RQ;
        else if(where_in_cache == 2)    return hit_where_t::L1I_WQ;
        else if(where_in_cache == 3)    return hit_where_t::L1I_MSHR;
    }
    else if(cache_type == IS_L1D)
    {
        if(where_in_cache == 0)         return hit_where_t::L1D;
        else if(where_in_cache == 1)    return hit_where_t::L1D_RQ;
        else if(where_in_cache == 2)    return hit_where_t::L1D_WQ;
        else if(where_in_cache == 3)    return hit_where_t::L1D_MSHR;
    }
    else if(cache_type == IS_L2C)
    {
        if(where_in_cache == 0)         return hit_where_t::L2C;
        else if(where_in_cache == 1)    return hit_where_t::L2C_RQ;
        else if(where_in_cache == 2)    return hit_where_t::L2C_WQ;
        else if(where_in_cache == 3)    return hit_where_t::L2C_MSHR;
    }
    else if(cache_type == IS_LLC)
    {
        if(where_in_cache == 0)         return hit_where_t::LLC;
        else if(where_in_cache == 1)    return hit_where_t::LLC_RQ;
        else if(where_in_cache == 2)    return hit_where_t::LLC_WQ;
        else if(where_in_cache == 3)    return hit_where_t::LLC_MSHR;
    }

    return hit_where_t::INV;
}

void CACHE::send_signal_to_core(uint32_t cpu, PACKET packet)
{
    uint32_t lq_index = packet.lq_index;
    // uint32_t source_index = ooo_cpu[cpu].get_source_index_from_rob(rob_index, lq_index);

#ifdef SANITY_CHECK
    uint32_t rob_index = packet.rob_index;
    if(ooo_cpu[cpu].LQ.entry[lq_index].rob_index != rob_index)
    {
        cout << "rob_index in LQ entry does not match with the rob_index from RQ entry"
            << " rob_index from LQ: " << ooo_cpu[cpu].LQ.entry[lq_index].rob_index
            << " rob_index from RQ: " << rob_index << endl;
        assert(0);
    }
#endif

    ooo_cpu[cpu].LQ.entry[lq_index].went_offchip = 1; // mark in LQ

    // send signal to all merged loads too
    if (knob::offchip_pred_mark_merged_load)
    {
        ITERATE_SET(merged, packet.lq_index_depend_on_me, ooo_cpu[cpu].LQ.SIZE)
        {
            ooo_cpu[cpu].LQ.entry[merged].went_offchip = 1;
        }
    }
}

void CACHE::broadcast_bw(uint8_t bw_level)
{
    /* boradcast to all the attached prefetchers */
    switch(cache_type)
    {
        case IS_L1I:
            break;
        case IS_L1D:
            l1d_prefetcher_broadcast_bw(bw_level);
            break;
        case IS_L2C:
            l2c_prefetcher_broadcast_bw(bw_level);
            break;
        case IS_LLC:
            llc_prefetcher_broadcast_bw(bw_level);
            break;
    }

    /* recursively broadcast to higher caches */
    CACHE *cache = NULL;
    for(uint32_t core = 0; core < NUM_CPUS; ++core)
    {
        if(upper_level_dcache[core])
        {
            cache = (CACHE*)upper_level_dcache[core];
            cache->broadcast_bw(bw_level);
        }
        if(upper_level_icache[core] && upper_level_icache[core] != upper_level_dcache[core])
        {
            cache = (CACHE*)upper_level_icache[core];
            cache->broadcast_bw(bw_level);
        }
    }
}

void CACHE::broadcast_ipc(uint8_t ipc)
{
    if (cache_type == IS_L1D)
        l1d_prefetcher_broadcast_ipc(ipc);
    else if (cache_type == IS_L2C)
        l2c_prefetcher_broadcast_ipc(ipc);
    else if (cache_type == IS_LLC)
        llc_prefetcher_broadcast_ipc(ipc);
}

bool CACHE::search_and_add(uint64_t page)
{
    bool found = false;
    auto it = find_if(page_buffer.begin(), page_buffer.end(), [page](uint64_t p){return p == page;});
    if(it != page_buffer.end()) found = true;
    if(!found)
    {
        if(page_buffer.size() >= knob::semi_perfect_cache_page_buffer_size)
        {
            page_buffer.pop_front();
        }
        page_buffer.push_back(page);
    }
    return found;
}

void CACHE::handle_prefetch_feedback()
{
    uint32_t this_epoch_accuracy = 0, acc_level = 0;

    cycle++;
    if(knob::measure_cache_acc && cycle >= next_measure_cycle)
    {
        this_epoch_accuracy = pf_filled_epoch ? 100*(float)pf_useful_epoch/pf_filled_epoch : 0; 
        pref_acc = (pref_acc + this_epoch_accuracy) / 2; // have some hysterisis
        acc_level = (pref_acc / ((float)100/CACHE_ACC_LEVELS)); // quantize into 8 buckets
        if(acc_level >= CACHE_ACC_LEVELS) acc_level = (CACHE_ACC_LEVELS - 1); // corner cases

        pf_useful_epoch = 0;
        pf_filled_epoch = 0;
        next_measure_cycle = cycle + knob::measure_cache_acc_epoch;

        total_acc_epochs++;
        acc_epoch_hist[acc_level]++;

        broadcast_acc(acc_level);
    }
}

void CACHE::broadcast_acc(uint32_t acc_level)
{
    /* boradcast to all the attached prefetchers */
    switch(cache_type)
    {
        case IS_L1I:    return; 
        case IS_L1D:    return l1d_prefetcher_broadcast_acc(acc_level);
        case IS_L2C:    return l2c_prefetcher_broadcast_acc(acc_level);
        case IS_LLC:    return llc_prefetcher_broadcast_acc(acc_level);
    }
}