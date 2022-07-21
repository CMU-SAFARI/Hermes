#include <algorithm>
#include "dram_controller.h"
#include "ooo_cpu.h"
#include "uncore.h"
#include "util.h"

namespace knob
{
    extern bool     enable_pseudo_direct_dram_prefetch;
    extern bool     enable_pseudo_direct_dram_prefetch_on_prefetch;
    extern uint32_t pseudo_direct_dram_prefetch_rob_part_type;
    extern bool     enable_ddrp;
    extern uint32_t dram_rq_schedule_type;
    extern bool     dram_force_rq_row_buffer_miss;
    extern bool     dram_cntlr_enable_ddrp_buffer;
    extern uint32_t dram_cntlr_ddrp_buffer_sets;
    extern uint32_t dram_cntlr_ddrp_buffer_assoc;
    extern uint32_t dram_cntlr_ddrp_buffer_hash_type;
}

// initialized in main.cc
uint32_t DRAM_MTPS, DRAM_DBUS_RETURN_TIME, DRAM_DBUS_MAX_CAS,
         tRP, tRCD, tCAS;

string MemControllerScheduleTypeString[] = {
    "FR_FCFS",
    "FRONTAL_FR_FCFS",
    "FR_FRONTAL_FCFS",
    "ROB_PART_FR_FCFS",
    "FR_ROB_PART_FCFS",
    "CR_FCFS"
};

void print_dram_config()
{
    cout << "dram_channel_width " << DRAM_CHANNEL_WIDTH << endl
        << "dram_wq_size " << DRAM_WQ_SIZE << endl
        << "dram_rq_size " << DRAM_RQ_SIZE << endl
        << "tRP " << tRP_DRAM_NANOSECONDS << endl
        << "tRCD " << tRCD_DRAM_NANOSECONDS << endl
        << "tCAS " << tCAS_DRAM_NANOSECONDS << endl
        << "dram_dbus_turn_around_time " << DRAM_DBUS_TURN_AROUND_TIME << endl
        << "dram_write_high_wm " << DRAM_WRITE_HIGH_WM << endl
        << "dram_write_low_wm " << DRAM_WRITE_LOW_WM << endl
        << "min_dram_writes_per_switch " << MIN_DRAM_WRITES_PER_SWITCH << endl
        << "dram_mtps " << DRAM_MTPS << endl
        << "dram_dbus_return_time " << DRAM_DBUS_RETURN_TIME << endl
        << "dram_rq_schedule_type " << MemControllerScheduleTypeString[knob::dram_rq_schedule_type] << endl
        << "dram_force_rq_row_buffer_miss " << knob::dram_force_rq_row_buffer_miss << endl
        << endl;
}

void MEMORY_CONTROLLER::reset_remain_requests(PACKET_QUEUE *queue, uint32_t channel)
{
    for (uint32_t i=0; i<queue->SIZE; i++) {
        if (queue->entry[i].scheduled) {

            uint64_t op_addr = queue->entry[i].address;
            uint32_t op_cpu = queue->entry[i].cpu,
                     op_channel = dram_get_channel(op_addr), 
                     op_rank = dram_get_rank(op_addr), 
                     op_bank = dram_get_bank(op_addr), 
                     op_row = dram_get_row(op_addr);

#ifdef DEBUG_PRINT
            //uint32_t op_column = dram_get_column(op_addr);
#endif

            // update open row
            if ((bank_request[op_channel][op_rank][op_bank].cycle_available - tCAS) <= current_core_cycle[op_cpu])
                bank_request[op_channel][op_rank][op_bank].open_row = op_row;
            else
                bank_request[op_channel][op_rank][op_bank].open_row = UINT32_MAX;

            // this bank is ready for another DRAM request
            bank_request[op_channel][op_rank][op_bank].request_index = -1;
            bank_request[op_channel][op_rank][op_bank].row_buffer_hit = 0;
            bank_request[op_channel][op_rank][op_bank].working = 0;
            bank_request[op_channel][op_rank][op_bank].cycle_available = current_core_cycle[op_cpu];
            if (bank_request[op_channel][op_rank][op_bank].is_write) {
                scheduled_writes[channel]--;
                bank_request[op_channel][op_rank][op_bank].is_write = 0;
            }
            else if (bank_request[op_channel][op_rank][op_bank].is_read) {
                scheduled_reads[channel]--;
                bank_request[op_channel][op_rank][op_bank].is_read = 0;
            }

            queue->entry[i].scheduled = 0;
            queue->entry[i].event_cycle = current_core_cycle[op_cpu];

            DP ( if (warmup_complete[op_cpu]) {
            cout << queue->NAME << " instr_id: " << queue->entry[i].instr_id << " swrites: " << scheduled_writes[channel] << " sreads: " << scheduled_reads[channel] << endl; });

        }
    }
    
    update_schedule_cycle(&RQ[channel]);
    update_schedule_cycle(&WQ[channel]);
    update_process_cycle(&RQ[channel]);
    update_process_cycle(&WQ[channel]);

#ifdef SANITY_CHECK
    if (queue->is_WQ) {
        if (scheduled_writes[channel] != 0)
            assert(0);
    }
    else {
        if (scheduled_reads[channel] != 0)
            assert(0);
    }
#endif
}

void MEMORY_CONTROLLER::operate()
{
    for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
        //if ((write_mode[i] == 0) && (WQ[i].occupancy >= DRAM_WRITE_HIGH_WM)) {
      if ((write_mode[i] == 0) && ((WQ[i].occupancy >= DRAM_WRITE_HIGH_WM) || ((RQ[i].occupancy == 0) && (WQ[i].occupancy > 0)))) { // use idle cycles to perform writes
            write_mode[i] = 1;

            // reset scheduled RQ requests
            reset_remain_requests(&RQ[i], i);
            // add data bus turn-around time
            dbus_cycle_available[i] += DRAM_DBUS_TURN_AROUND_TIME;
        } else if (write_mode[i]) {

            if (WQ[i].occupancy == 0)
                write_mode[i] = 0;
            else if (RQ[i].occupancy && (WQ[i].occupancy < DRAM_WRITE_LOW_WM))
                write_mode[i] = 0;

            if (write_mode[i] == 0) {
                // reset scheduled WQ requests
                reset_remain_requests(&WQ[i], i);
                // add data bus turnaround time
                dbus_cycle_available[i] += DRAM_DBUS_TURN_AROUND_TIME;
            }
        }

        // handle write
        // schedule new entry
        if (write_mode[i] && (WQ[i].next_schedule_index < WQ[i].SIZE)) {
            if (WQ[i].next_schedule_cycle <= current_core_cycle[WQ[i].entry[WQ[i].next_schedule_index].cpu])
                schedule(&WQ[i]);
        }

        // process DRAM requests
        if (write_mode[i] && (WQ[i].next_process_index < WQ[i].SIZE)) {
            if (WQ[i].next_process_cycle <= current_core_cycle[WQ[i].entry[WQ[i].next_process_index].cpu])
                process(&WQ[i]);
        }

        // handle read
        // schedule new entry
        if ((write_mode[i] == 0) && (RQ[i].next_schedule_index < RQ[i].SIZE)) {
            if (RQ[i].next_schedule_cycle <= current_core_cycle[RQ[i].entry[RQ[i].next_schedule_index].cpu])
                schedule(&RQ[i]);
        }

        // process DRAM requests
        if ((write_mode[i] == 0) && (RQ[i].next_process_index < RQ[i].SIZE)) {
            if (RQ[i].next_process_cycle <= current_core_cycle[RQ[i].entry[RQ[i].next_process_index].cpu])
                process(&RQ[i]);
        }
    }
}

void MEMORY_CONTROLLER::schedule(PACKET_QUEUE *queue)
{
    uint8_t  row_buffer_hit = 0;
    int index = find_next_request(queue, row_buffer_hit);

    if(queue->is_RQ && knob::dram_force_rq_row_buffer_miss)
    {
        row_buffer_hit = 0;
    }

    // at this point, the scheduler knows which bank to access and if the request is a row buffer hit or miss
    if (index != -1) // scheduler might not find anything if all requests are already scheduled or all banks are busy
    { 
        uint64_t op_addr = queue->entry[index].address;
        uint32_t op_cpu = queue->entry[index].cpu,
                 op_rob_pos = queue->entry[index].rob_position,
                 op_channel = dram_get_channel(op_addr), 
                 op_rank = dram_get_rank(op_addr), 
                 op_bank = dram_get_bank(op_addr), 
                 op_row = dram_get_row(op_addr);
#ifdef DEBUG_PRINT
        uint32_t op_column = dram_get_column(op_addr);
#endif

        uint64_t LATENCY = 0;
        if (row_buffer_hit)  
            LATENCY = tCAS;
        else 
            LATENCY = tRP + tRCD + tCAS;

        // model pseudo direct DRAM prefetch
        if(knob::enable_pseudo_direct_dram_prefetch && queue->entry[index].is_data)
        {
            // model only for loads, and also for prefetch requests if enabled
            if(queue->entry[index].type == LOAD || (queue->entry[index].type == PREFETCH && knob::enable_pseudo_direct_dram_prefetch_on_prefetch))
            {
                uint8_t op_rob_part_type = ooo_cpu[op_cpu].rob_pos_get_part_type(op_rob_pos);
                if(knob::pseudo_direct_dram_prefetch_rob_part_type == NUM_PARTITION_TYPES 
                    || (uint32_t)op_rob_part_type == knob::pseudo_direct_dram_prefetch_rob_part_type)
                {
                    uint64_t on_chip_cache_lookup_lat = L1D_LATENCY + L2C_LATENCY + LLC_LATENCY;
                    if(LATENCY > on_chip_cache_lookup_lat)
                    {
                        LATENCY = LATENCY - on_chip_cache_lookup_lat;
                        stats.pseudo_direct_dram_prefetch.reduced_lat++;
                    }
                    else
                    {
                        // the real headroom might be even higher than this
                        // when total on-chip cache lookup latency will be much higher 
                        // than the row buffer hit latency
                        LATENCY = 0;
                        stats.pseudo_direct_dram_prefetch.zero_lat++;
                    }
                }
            }
        }

        // this bank is now busy
        bank_request[op_channel][op_rank][op_bank].working = 1;
        bank_request[op_channel][op_rank][op_bank].working_type = queue->entry[index].type;
        bank_request[op_channel][op_rank][op_bank].cycle_available = current_core_cycle[op_cpu] + LATENCY;

        bank_request[op_channel][op_rank][op_bank].request_index = index;
        bank_request[op_channel][op_rank][op_bank].row_buffer_hit = row_buffer_hit;
        if (queue->is_WQ) 
        {
            bank_request[op_channel][op_rank][op_bank].is_write = 1;
            bank_request[op_channel][op_rank][op_bank].is_read = 0;
            scheduled_writes[op_channel]++;
        }
        else 
        {
            bank_request[op_channel][op_rank][op_bank].is_write = 0;
            bank_request[op_channel][op_rank][op_bank].is_read = 1;
            scheduled_reads[op_channel]++;
        }

        // update open row
        bank_request[op_channel][op_rank][op_bank].open_row = op_row;

        queue->entry[index].scheduled = 1;
        queue->entry[index].event_cycle = current_core_cycle[op_cpu] + LATENCY;

        if(queue->entry[index].is_data && queue->entry[index].type == LOAD)
        {
            stats.data_loads.total_loads++;
            uint8_t op_rob_part_type = ooo_cpu[op_cpu].rob_pos_get_part_type(op_rob_pos);
            stats.data_loads.load_cat[op_rob_part_type]++;
        }

        update_schedule_cycle(queue);
        update_process_cycle(queue);

        DP (if (warmup_complete[op_cpu]) {
        cout << "[" << queue->NAME << "] " <<  __func__ << " instr_id: " << queue->entry[index].instr_id;
        cout << " row buffer: " << (row_buffer_hit ? (int)bank_request[op_channel][op_rank][op_bank].open_row : -1) << hex;
        cout << " address: " << queue->entry[index].address << " full_addr: " << queue->entry[index].full_addr << dec;
        cout << " index: " << index << " occupancy: " << queue->occupancy;
        cout << " ch: " << op_channel << " rank: " << op_rank << " bank: " << op_bank; // wrong from here
        cout << " row: " << op_row << " col: " << op_column;
        cout << " current: " << current_core_cycle[op_cpu] << " event: " << queue->entry[index].event_cycle << endl; });
    }
}

int MEMORY_CONTROLLER::find_next_request(PACKET_QUEUE *queue, uint8_t& row_buffer_hit)
{
    if(queue->is_WQ)
    {
        return schedule_FR_FCFS(queue, row_buffer_hit);
    }
    else
    {
        switch(knob::dram_rq_schedule_type)
        {
            case FR_FCFS:               return schedule_FR_FCFS(queue, row_buffer_hit);
            case FRONTAL_FR_FCFS:       return schedule_FRONTAL_FR_FCFS(queue, row_buffer_hit);
            case FR_FRONTAL_FCFS:       return schedule_FR_FRONTAL_FCFS(queue, row_buffer_hit);
            case ROB_PART_FR_FCFS:      return schedule_ROB_PART_FR_FCFS(queue, row_buffer_hit);
            case FR_ROB_PART_FCFS:      return schedule_FR_ROB_PART_FCFS(queue, row_buffer_hit);
            case CR_FCFS:               return schedule_CR_FCFS(queue, row_buffer_hit);
            default:                    return -1;
        }
    }
}

void MEMORY_CONTROLLER::process(PACKET_QUEUE *queue)
{
    uint32_t request_index = queue->next_process_index;

    // sanity check
    if (request_index == queue->SIZE)
        assert(0);

    uint8_t  op_type = queue->entry[request_index].type;
    uint64_t op_addr = queue->entry[request_index].address;
    uint32_t op_cpu = queue->entry[request_index].cpu,
             op_channel = dram_get_channel(op_addr), 
             op_rank = dram_get_rank(op_addr), 
             op_bank = dram_get_bank(op_addr);
#ifdef DEBUG_PRINT
    uint32_t op_row = dram_get_row(op_addr), 
             op_column = dram_get_column(op_addr);
#endif

    // sanity check
    if (bank_request[op_channel][op_rank][op_bank].request_index != (int)request_index) 
    {
        assert(0);
    }

    // paid all DRAM access latency, data is ready to be processed
    if (bank_request[op_channel][op_rank][op_bank].cycle_available <= current_core_cycle[op_cpu]) 
    {
        // check if data bus is available
        if (dbus_cycle_available[op_channel] <= current_core_cycle[op_cpu]) 
        {
            if (queue->is_WQ) 
            {
                // update data bus cycle time
                dbus_cycle_available[op_channel] = current_core_cycle[op_cpu] + DRAM_DBUS_RETURN_TIME;

                if (bank_request[op_channel][op_rank][op_bank].row_buffer_hit)
                    queue->ROW_BUFFER_HIT++;
                else
                    queue->ROW_BUFFER_MISS++;

                // this bank is ready for another DRAM request
                bank_request[op_channel][op_rank][op_bank].request_index = -1;
                bank_request[op_channel][op_rank][op_bank].row_buffer_hit = 0;
                bank_request[op_channel][op_rank][op_bank].working = false;
                bank_request[op_channel][op_rank][op_bank].is_write = 0;
                bank_request[op_channel][op_rank][op_bank].is_read = 0;

                scheduled_writes[op_channel]--;
            } 
            else 
            {
                // update data bus cycle time
                dbus_cycle_available[op_channel] = current_core_cycle[op_cpu] + DRAM_DBUS_RETURN_TIME;
                queue->entry[request_index].event_cycle = dbus_cycle_available[op_channel]; 

                DP ( if (warmup_complete[op_cpu]) {
                cout << "[" << queue->NAME << "] " <<  __func__ << " return data" << hex;
                cout << " address: " << queue->entry[request_index].address << " full_addr: " << queue->entry[request_index].full_addr << dec;
                cout << " occupancy: " << queue->occupancy << " channel: " << op_channel << " rank: " << op_rank << " bank: " << op_bank;
                cout << " row: " << op_row << " column: " << op_column;
                cout << " current_cycle: " << current_core_cycle[op_cpu] << " event_cycle: " << queue->entry[request_index].event_cycle << endl; });

                // send data back to the core cache hierarchy, only if the request is not DDRP
                if(queue->entry[request_index].fill_level < FILL_DDRP)
                {
                    upper_level_dcache[op_cpu]->return_data(&queue->entry[request_index]);
                    stats.dram_process.returned[queue->entry[request_index].type]++;

                    DDRP_DP ( if (warmup_complete[op_cpu]) {
                    cout << "[" << queue->NAME << "] " <<  __func__ << " return data";
                    cout << " instr_id: " << queue->entry[request_index].instr_id << " address: " << hex << queue->entry[request_index].address << dec;
                    cout << " full_addr: " << hex << queue->entry[request_index].full_addr << dec << " type: " << +queue->entry[request_index].type << " fill_level: " << queue->entry[request_index].fill_level;
                    cout << " current_cycle: " << current_core_cycle[op_cpu] << " event_cycle: " << queue->entry[request_index].event_cycle << endl; });
                }
                else
                {
                    assert(queue->entry[request_index].type == PREFETCH); // this has to be a DDRP prefetch
                    // if DDRP buffer is enabled, put this otherwise wasted request there
                    if(knob::dram_cntlr_enable_ddrp_buffer)
                    {
                        stats.dram_process.buffered++;
                        insert_ddrp_buffer(op_addr);
                        DDRP_DP ( if (warmup_complete[op_cpu]) {
                        cout << "[" << queue->NAME << "] " <<  __func__ << " buffering_data";
                        cout << " instr_id: " << queue->entry[request_index].instr_id << " address: " << hex << queue->entry[request_index].address << dec;
                        cout << " full_addr: " << hex << queue->entry[request_index].full_addr << dec << " type: " << +queue->entry[request_index].type << " fill_level: " << queue->entry[request_index].fill_level;
                        cout << " current_cycle: " << current_core_cycle[op_cpu] << " event_cycle: " << queue->entry[request_index].event_cycle << endl; });
                    }
                    else
                    {
                        // request is wasted
                        stats.dram_process.not_returned++;
                    }
                }

                if (bank_request[op_channel][op_rank][op_bank].row_buffer_hit)
                    queue->ROW_BUFFER_HIT++;
                else
                    queue->ROW_BUFFER_MISS++;

                // this bank is ready for another DRAM request
                bank_request[op_channel][op_rank][op_bank].request_index = -1;
                bank_request[op_channel][op_rank][op_bank].row_buffer_hit = 0;
                bank_request[op_channel][op_rank][op_bank].working = false;
                bank_request[op_channel][op_rank][op_bank].is_write = 0;
                bank_request[op_channel][op_rank][op_bank].is_read = 0;

                scheduled_reads[op_channel]--;
            }

            // remove the oldest entry
            // cout << "[DEQUEUE_" << queue->NAME << "] " << " id: " << queue->entry[request_index].id << " cpu: " << queue->entry[request_index].cpu << " instr_id: " << queue->entry[request_index].instr_id;
            // cout << " address: " << hex << queue->entry[request_index].address << " full_addr: " << queue->entry[request_index].full_addr << dec;
            // cout << " instruction: " << (uint32_t)queue->entry[request_index].instruction << " is_data: " << (uint32_t)queue->entry[request_index].is_data;
            // cout << " timestamp: " << uncore.cycle;
            // cout << " entry_enq_timestamp: " << queue->entry[request_index].enque_cycle[queue->module_type][queue->queue_type];
            // cout << " entry_deq_timestamp: " << queue->entry[request_index].deque_cycle[queue->module_type][queue->queue_type] << endl;
            queue->remove_queue(&queue->entry[request_index], uncore.cycle);
            update_process_cycle(queue);
        }
        else 
        { 
            // data bus is busy, the available bank cycle time is fast-forwarded for faster simulation

            dbus_cycle_congested[op_channel] += (dbus_cycle_available[op_channel] - current_core_cycle[op_cpu]);
            bank_request[op_channel][op_rank][op_bank].cycle_available = dbus_cycle_available[op_channel];
            dbus_congested[op_channel][NUM_TYPES][NUM_TYPES]++;
            dbus_congested[op_channel][NUM_TYPES][op_type]++;
            dbus_congested[op_channel][bank_request[op_channel][op_rank][op_bank].working_type][NUM_TYPES]++;
            dbus_congested[op_channel][bank_request[op_channel][op_rank][op_bank].working_type][op_type]++;

            DP ( if (warmup_complete[op_cpu]) {
            cout << "[" << queue->NAME << "] " <<  __func__ << " dbus_occupied" << hex;
            cout << " address: " << queue->entry[request_index].address << " full_addr: " << queue->entry[request_index].full_addr << dec;
            cout << " occupancy: " << queue->occupancy << " channel: " << op_channel << " rank: " << op_rank << " bank: " << op_bank;
            cout << " row: " << op_row << " column: " << op_column;
            cout << " current_cycle: " << current_core_cycle[op_cpu] << " event_cycle: " << bank_request[op_channel][op_rank][op_bank].cycle_available << endl; });
        }
    }
}

int MEMORY_CONTROLLER::add_rq(PACKET *packet)
{
    bool return_data_to_core = true;
    if(knob::enable_ddrp && packet->fill_level >= FILL_DDRP)
    {
        return_data_to_core = false;
    }

    // simply return read requests with dummy response before the warmup
    if (all_warmup_complete < NUM_CPUS && return_data_to_core) 
    {
        if (packet->instruction) 
            upper_level_icache[packet->cpu]->return_data(packet);
        if (packet->is_data)
            upper_level_dcache[packet->cpu]->return_data(packet);

        DDRP_DP ( if(warmup_complete[packet->cpu]) {
        cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << packet->instr_id << " address: " << hex << packet->address;
        cout << " full_addr: " << packet->full_addr << dec << " type: " << +packet->type << " fill_level: " << packet->fill_level << " return_data_before_warmup" << endl; });

        return -1;
    }

    // check for the latest wirtebacks in the write queue
    uint32_t channel = dram_get_channel(packet->address);
    int wq_index = check_dram_queue(&WQ[channel], packet);
    if (wq_index != -1) 
    {
        if (return_data_to_core) 
        {
            packet->data = WQ[channel].entry[wq_index].data;
            if (packet->instruction) 
                upper_level_icache[packet->cpu]->return_data(packet);
            if (packet->is_data) 
                upper_level_dcache[packet->cpu]->return_data(packet);
        }

        DP ( if (packet->cpu) {
        cout << "[" << NAME << "_RQ] " << __func__ << " instr_id: " << packet->instr_id << " found recent writebacks";
        cout << hex << " read: " << packet->address << " writeback: " << WQ[channel].entry[wq_index].address << dec << endl; });

        ACCESS[1]++;
        HIT[1]++;

        WQ[channel].FORWARD++;
        RQ[channel].ACCESS++;

        DDRP_DP ( if(warmup_complete[packet->cpu]) {
        cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << packet->instr_id << " address: " << hex << packet->address;
        cout << " full_addr: " << packet->full_addr << dec << " DRAM_WQ_" << channel << "_FORWARD"; });

        return -1;
    }

    // check for possible hit in DDRP buffer
    if(knob::dram_cntlr_enable_ddrp_buffer)
    {
        bool hit = lookup_ddrp_buffer(packet->address);
        if(hit)
        {
            // RBERA_TODO: do we want to model latency here?
            if (return_data_to_core) 
            {
                // RBERA_TODO: what should be the data payload of the packet?
                if (packet->instruction) 
                    upper_level_icache[packet->cpu]->return_data(packet);
                if (packet->is_data) 
                    upper_level_dcache[packet->cpu]->return_data(packet);

                stats.dram_process.returned[packet->type]++;
            }

            DDRP_DP ( if(warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << packet->instr_id << " address: " << hex << packet->address;
            cout << " full_addr: " << packet->full_addr << dec << " DDRP_BUFFER_FORWARD"; });

            if(packet->fill_level <= FILL_LLC)
            {
                stats.ddrp.llc_miss.ddrp_buffer_hit[packet->type]++;
                stats.ddrp.llc_miss.total[packet->type]++;
            }
            else if(packet->fill_level == FILL_DDRP)
            {
                stats.ddrp.ddrp_req.ddrp_buffer_hit++;
                stats.ddrp.ddrp_req.total++;
            }

            return -1;
        }
    }

    // check for duplicates in the read queue
    int index = check_dram_queue(&RQ[channel], packet);
    // request should not merge in DRAM's RQ, unless DDRP is turned on
    assert(index == -1 || knob::enable_ddrp);
    if (index != -1)
    {
        DDRP_DP ( if(warmup_complete[packet->cpu]) {
        cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << packet->instr_id << " address: " << hex << packet->address;
        cout << " full_addr: " << packet->full_addr << dec << " LLC_MISS_MERGE_IN_RQ";
        cout << " occupancy: " << RQ[channel].occupancy << " current: " << current_core_cycle[packet->cpu] << " event: " << packet->event_cycle << endl; });

        stats.rq_merged++;
        
        if(RQ[channel].entry[index].fill_level == FILL_DDRP)
        {
            if(packet->fill_level <= FILL_LLC) // incoming LLC miss
            {
                // RBERA_TODO: properly merge the incoming LLC miss to
                // the exsisting DDRP request so that we can correctly
                // return data back to the core
                uint8_t tmp_scheduled = RQ[channel].entry[index].scheduled;
                uint64_t tmp_event_cycle = RQ[channel].entry[index].event_cycle;
                uint64_t tmp_enque_cycle = RQ[channel].entry[index].enque_cycle[IS_DRAM][IS_RQ];
                RQ[channel].entry[index] = *packet; // merge
                RQ[channel].entry[index].scheduled = tmp_scheduled;
                RQ[channel].entry[index].event_cycle = tmp_event_cycle;
                RQ[channel].entry[index].enque_cycle[IS_DRAM][IS_RQ] = tmp_enque_cycle;
                stats.ddrp.llc_miss.rq_hit[RQ[channel].entry[index].type]++;
                stats.ddrp.llc_miss.total[RQ[channel].entry[index].type]++;
            }
            else if(packet->fill_level == FILL_DDRP) // incoming DDRP request
            {
                ; // no need to do anything, as
                // this is a DDRP request hitting
                // another DDRP request in RQ
                stats.ddrp.ddrp_req.rq_hit[0]++;
                stats.ddrp.ddrp_req.total++;
            }
            else{
                assert(false);
                return -2;
            }
        }
        else if(RQ[channel].entry[index].fill_level <= FILL_LLC)
        {
            if(packet->fill_level <= FILL_LLC) // incoming LLC miss
            {
                // the incoming packet and RQ's packet both cannot be LLC miss requests
                assert(false);
                return -2;
            }
            else if(packet->fill_level == FILL_DDRP) // incoming DDRP request
            {
                ; // no need to upgrade the request, 
                // as the request that already went
                // t0 DRAM is requested by core.
                stats.ddrp.ddrp_req.rq_hit[1]++;
                stats.ddrp.ddrp_req.total++;
            }
            else{
                assert(false);
                return -2;
            }
        }
        else{
            assert(false);
            return -2;
        }

        return index; // merged index
    }

    // search for the empty index
    for (uint32_t index=0; index<DRAM_RQ_SIZE; index++) 
    {
        if (RQ[channel].entry[index].address == 0) 
        {    
            RQ[channel].entry[index] = *packet;
            RQ[channel].occupancy++;
            RQ[channel].entry[index].enque_cycle[IS_DRAM][IS_RQ] = uncore.cycle;
            rq_enqueue_count++;

            // cout << "[ENQUEUE_" << RQ[channel].NAME << "] " << " id: " << packet->id << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
            // cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
            // cout << " instruction: " << (uint32_t)packet->instruction << " is_data: " << (uint32_t)packet->is_data; 
            // cout << " timestamp: " << uncore.cycle;
            // cout << " packet_enq_timestamp: " << packet->enque_cycle[IS_DRAM][IS_RQ] << " packet_deq_timestamp: " << packet->deque_cycle[IS_DRAM][IS_RQ];
            // cout << " entry_enq_timestamp: " << RQ[channel].entry[index].enque_cycle[IS_DRAM][IS_RQ] << " entry_deq_timestamp: " << RQ[channel].entry[index].deque_cycle[IS_DRAM][IS_RQ] << endl;

#ifdef DEBUG_PRINT
            uint32_t channel = dram_get_channel(packet->address),
                        rank = dram_get_rank(packet->address),
                        bank = dram_get_bank(packet->address),
                        row = dram_get_row(packet->address),
                        column = dram_get_column(packet->address); 
#endif

            DP ( if(warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << packet->instr_id << " address: " << hex << packet->address;
            cout << " full_addr: " << packet->full_addr << dec << " ch: " << channel;
            cout << " rank: " << rank << " bank: " << bank << " row: " << row << " col: " << column;
            cout << " occupancy: " << RQ[channel].occupancy << " current: " << current_core_cycle[packet->cpu] << " event: " << packet->event_cycle << endl; });

            DDRP_DP ( if(warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "_RQ] " <<  __func__ << " instr_id: " << packet->instr_id << " address: " << hex << packet->address;
            cout << " full_addr: " << packet->full_addr << dec  << " type: " << +packet->type << " fill_level: " << packet->fill_level << " ADDED_IN_DRAM_RQ_" << channel;
            cout << " occupancy: " << RQ[channel].occupancy << " current: " << current_core_cycle[packet->cpu] << " event: " << packet->event_cycle << endl; });

            break;
        }
    }

    if(packet->fill_level <= FILL_LLC)
    {
        stats.ddrp.llc_miss.went_to_dram[packet->type]++;
        stats.ddrp.llc_miss.total[packet->type]++;
    }
    else if(packet->fill_level == FILL_DDRP)
    {
        stats.ddrp.ddrp_req.went_to_dram++;
        stats.ddrp.ddrp_req.total++;
    }

    update_schedule_cycle(&RQ[channel]);
    return -1;
}

int MEMORY_CONTROLLER::add_wq(PACKET *packet)
{
    // simply drop write requests before the warmup
    if (all_warmup_complete < NUM_CPUS)
        return -1;

    // check for duplicates in the write queue
    uint32_t channel = dram_get_channel(packet->address);
    int index = check_dram_queue(&WQ[channel], packet);
    if (index != -1)
        return index; // merged index

    // search for the empty index
    for (index=0; index<DRAM_WQ_SIZE; index++) {
        if (WQ[channel].entry[index].address == 0) {
            
            WQ[channel].entry[index] = *packet;
            WQ[channel].occupancy++;
            WQ[channel].entry[index].enque_cycle[IS_DRAM][IS_WQ] = uncore.cycle;
            
            // cout << "[ENQUEUE_" << WQ[channel].NAME << "] " << " id: " << packet->id << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
            // cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
            // cout << " instruction: " << (uint32_t)packet->instruction << " is_data: " << (uint32_t)packet->is_data; 
            // cout << " timestamp: " << uncore.cycle;
            // cout << " packet_enq_timestamp: " << packet->enque_cycle[IS_DRAM][IS_WQ] << " packet_deq_timestamp: " << packet->deque_cycle[IS_DRAM][IS_WQ];
            // cout << " entry_enq_timestamp: " << WQ[channel].entry[index].enque_cycle[IS_DRAM][IS_WQ] << " entry_deq_timestamp: " << WQ[channel].entry[index].deque_cycle[IS_DRAM][IS_WQ] << endl;

#ifdef DEBUG_PRINT
            uint32_t channel = dram_get_channel(packet->address),
                     rank = dram_get_rank(packet->address),
                     bank = dram_get_bank(packet->address),
                     row = dram_get_row(packet->address),
                     column = dram_get_column(packet->address); 
#endif

            DP ( if(warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "_WQ] " <<  __func__ << " instr_id: " << packet->instr_id << " address: " << hex << packet->address;
            cout << " full_addr: " << packet->full_addr << dec << " ch: " << channel;
            cout << " rank: " << rank << " bank: " << bank << " row: " << row << " col: " << column;
            cout << " occupancy: " << WQ[channel].occupancy << " current: " << current_core_cycle[packet->cpu] << " event: " << packet->event_cycle << endl; });

            break;
        }
    }

    update_schedule_cycle(&WQ[channel]);

    return -1;
}

int MEMORY_CONTROLLER::add_pq(PACKET *packet)
{
    return -1;
}

void MEMORY_CONTROLLER::return_data(PACKET *packet)
{

}

void MEMORY_CONTROLLER::update_schedule_cycle(PACKET_QUEUE *queue)
{
    // update next_schedule_cycle
    uint64_t min_cycle = UINT64_MAX;
    uint32_t min_index = queue->SIZE;
    for (uint32_t i=0; i<queue->SIZE; i++) {
        /*
        DP (if (warmup_complete[queue->entry[min_index].cpu]) {
        cout << "[" << queue->NAME << "] " <<  __func__ << " instr_id: " << queue->entry[i].instr_id;
        cout << " index: " << i << " address: " << hex << queue->entry[i].address << dec << " scheduled: " << +queue->entry[i].scheduled;
        cout << " event: " << queue->entry[i].event_cycle << " min_cycle: " << min_cycle << endl;
        });
        */

        if (queue->entry[i].address && (queue->entry[i].scheduled == 0) && (queue->entry[i].event_cycle < min_cycle)) {
            min_cycle = queue->entry[i].event_cycle;
            min_index = i;
        }
    }
    
    queue->next_schedule_cycle = min_cycle;
    queue->next_schedule_index = min_index;
    if (min_index < queue->SIZE) {

        DP (if (warmup_complete[queue->entry[min_index].cpu]) {
        cout << "[" << queue->NAME << "] " <<  __func__ << " instr_id: " << queue->entry[min_index].instr_id;
        cout << " address: " << hex << queue->entry[min_index].address << " full_addr: " << queue->entry[min_index].full_addr;
        cout << " data: " << queue->entry[min_index].data << dec;
        cout << " event: " << queue->entry[min_index].event_cycle << " current: " << current_core_cycle[queue->entry[min_index].cpu] << " next: " << queue->next_schedule_cycle << endl; });
    }
}

void MEMORY_CONTROLLER::update_process_cycle(PACKET_QUEUE *queue)
{
    // update next_process_cycle
    uint64_t min_cycle = UINT64_MAX;
    uint32_t min_index = queue->SIZE;
    for (uint32_t i=0; i<queue->SIZE; i++) {
        if (queue->entry[i].scheduled && (queue->entry[i].event_cycle < min_cycle)) {
            min_cycle = queue->entry[i].event_cycle;
            min_index = i;
        }
    }
    
    queue->next_process_cycle = min_cycle;
    queue->next_process_index = min_index;
    if (min_index < queue->SIZE) {

        DP (if (warmup_complete[queue->entry[min_index].cpu]) {
        cout << "[" << queue->NAME << "] " <<  __func__ << " instr_id: " << queue->entry[min_index].instr_id;
        cout << " address: " << hex << queue->entry[min_index].address << " full_addr: " << queue->entry[min_index].full_addr;
        cout << " data: " << queue->entry[min_index].data << dec << " num_returned: " << queue->num_returned;
        cout << " event: " << queue->entry[min_index].event_cycle << " current: " << current_core_cycle[queue->entry[min_index].cpu] << " next: " << queue->next_process_cycle << endl; });
    }
}

int MEMORY_CONTROLLER::check_dram_queue(PACKET_QUEUE *queue, PACKET *packet)
{
    // search write queue
    for (uint32_t index=0; index<queue->SIZE; index++) {
        if (queue->entry[index].address == packet->address) {
            
            DP ( if (warmup_complete[packet->cpu]) {
            cout << "[" << queue->NAME << "] " << __func__ << " same entry instr_id: " << packet->instr_id << " prior_id: " << queue->entry[index].instr_id;
            cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec << endl; });

            return index;
        }
    }

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << queue->NAME << "] " << __func__ << " new address: " << hex << packet->address;
    cout << " full_addr: " << packet->full_addr << dec << endl; });

    DP ( if (warmup_complete[packet->cpu] && (queue->occupancy == queue->SIZE)) {
    cout << "[" << queue->NAME << "] " << __func__ << " mshr is full";
    cout << " instr_id: " << packet->instr_id << " mshr occupancy: " << queue->occupancy;
    cout << " address: " << hex << packet->address;
    cout << " full_addr: " << packet->full_addr << dec;
    cout << " cycle: " << current_core_cycle[packet->cpu] << endl; });

    return -1;
}

uint32_t MEMORY_CONTROLLER::dram_get_channel(uint64_t address)
{
    if (LOG2_DRAM_CHANNELS == 0)
        return 0;

    int shift = 0;

    return (uint32_t) (address >> shift) & (DRAM_CHANNELS - 1);
}

uint32_t MEMORY_CONTROLLER::dram_get_bank(uint64_t address)
{
    if (LOG2_DRAM_BANKS == 0)
        return 0;

    int shift = LOG2_DRAM_CHANNELS;

    return (uint32_t) (address >> shift) & (DRAM_BANKS - 1);
}

uint32_t MEMORY_CONTROLLER::dram_get_column(uint64_t address)
{
    if (LOG2_DRAM_COLUMNS == 0)
        return 0;

    int shift = LOG2_DRAM_BANKS + LOG2_DRAM_CHANNELS;

    return (uint32_t) (address >> shift) & (DRAM_COLUMNS - 1);
}

uint32_t MEMORY_CONTROLLER::dram_get_rank(uint64_t address)
{
    if (LOG2_DRAM_RANKS == 0)
        return 0;

    int shift = LOG2_DRAM_COLUMNS + LOG2_DRAM_BANKS + LOG2_DRAM_CHANNELS;

    return (uint32_t) (address >> shift) & (DRAM_RANKS - 1);
}

uint32_t MEMORY_CONTROLLER::dram_get_row(uint64_t address)
{
    if (LOG2_DRAM_ROWS == 0)
        return 0;

    int shift = LOG2_DRAM_RANKS + LOG2_DRAM_COLUMNS + LOG2_DRAM_BANKS + LOG2_DRAM_CHANNELS;

    return (uint32_t) (address >> shift) & (DRAM_ROWS - 1);
}

uint32_t MEMORY_CONTROLLER::get_occupancy(uint8_t queue_type, uint64_t address)
{
    uint32_t channel = dram_get_channel(address);
    if (queue_type == 1)
        return RQ[channel].occupancy;
    else if (queue_type == 2)
        return WQ[channel].occupancy;

    return 0;
}

uint32_t MEMORY_CONTROLLER::get_size(uint8_t queue_type, uint64_t address)
{
    uint32_t channel = dram_get_channel(address);
    if (queue_type == 1)
        return RQ[channel].SIZE;
    else if (queue_type == 2)
        return WQ[channel].SIZE;

    return 0;
}

void MEMORY_CONTROLLER::increment_WQ_FULL(uint64_t address)
{
    uint32_t channel = dram_get_channel(address);
    WQ[channel].FULL++;
}

/* DDRP BUFFER */

void MEMORY_CONTROLLER::init_ddrp_buffer()
{
    // init buffer
    ddrp_buffer.clear();
    deque<uint64_t> d;
    ddrp_buffer.resize(knob::dram_cntlr_ddrp_buffer_sets, d);
}

void MEMORY_CONTROLLER::insert_ddrp_buffer(uint64_t addr)
{
    stats.ddrp_buffer.insert.called++;
    uint32_t set = get_ddrp_buffer_set_index(addr);
    auto it = find_if(ddrp_buffer[set].begin(), ddrp_buffer[set].end(), [addr](uint64_t m_addr){return m_addr == addr;});
    if(it != ddrp_buffer[set].end())
    {
        ddrp_buffer[set].erase(it);
        ddrp_buffer[set].push_back(addr);
        stats.ddrp_buffer.insert.hit++;
    }
    else
    {
        if(ddrp_buffer[set].size() >= knob::dram_cntlr_ddrp_buffer_assoc)
        {
            ddrp_buffer[set].pop_front();
            stats.ddrp_buffer.insert.evict++;
        }
        ddrp_buffer[set].push_back(addr);
        stats.ddrp_buffer.insert.insert++;
    }
}

bool MEMORY_CONTROLLER::lookup_ddrp_buffer(uint64_t addr)
{
    stats.ddrp_buffer.lookup.called++;
    uint32_t set = get_ddrp_buffer_set_index(addr);
    auto it = find_if(ddrp_buffer[set].begin(), ddrp_buffer[set].end(), [addr](uint64_t m_addr){return m_addr == addr;});
    if(it != ddrp_buffer[set].end())
    {
        stats.ddrp_buffer.lookup.hit++;
        return true;
    }
    else
    {
        stats.ddrp_buffer.lookup.miss++;
        return false;
    }
}

uint32_t MEMORY_CONTROLLER::get_ddrp_buffer_set_index(uint64_t address)
{
    uint32_t hash = folded_xor(address, 2);
    hash = HashZoo::getHash(knob::dram_cntlr_ddrp_buffer_hash_type, hash);
    return (hash % knob::dram_cntlr_ddrp_buffer_sets);
}