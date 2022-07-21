#include <iostream>
#include <sstream>
#include <iterator>
#include "block.h"

uint64_t PACKET::next_id = 0;

std::string PACKET::to_string()
{
    std::stringstream ss;
    ss << "ROB_id: " << rob_index
        << " ROB_pos: " << rob_position
        << " instr_id: " << instr_id
        << " is_ins: " << (uint32_t)instruction << " is_data: " << (uint32_t)is_data
        << " addr: " << hex << address << dec
        << " full_addr: " << hex << full_addr << dec
        << " fill_level: " << fill_level;
    return ss.str();
}

/******************************/
/*      base packet queue     */
/******************************/

int8_t PACKET_QUEUE_BASE::get_module_type()
{
    if (NAME.find("ITLB") != string::npos)          return IS_ITLB;
    else if (NAME.find("DTLB") != string::npos)     return IS_DTLB;
    else if (NAME.find("STLB") != string::npos)     return IS_STLB;
    else if (NAME.find("L1I") != string::npos)      return IS_L1I;
    else if (NAME.find("L1D") != string::npos)      return IS_L1D;
    else if (NAME.find("L2C") != string::npos)      return IS_L2C;
    else if (NAME.find("LLC") != string::npos)      return IS_LLC;
    else if (NAME.find("DRAM") != string::npos)     return IS_DRAM;
    return -1;
}

int8_t PACKET_QUEUE_BASE::get_queue_type()
{
    if (NAME.find("RQ") != string::npos)                return IS_RQ;
    else if (NAME.find("WQ") != string::npos)           return IS_WQ;
    else if (NAME.find("PQ") != string::npos)           return IS_PQ;
    else if (NAME.find("MSHR") != string::npos)         return IS_MSHR;
    else if (NAME.find("PROCESSED") != string::npos)    return IS_PROCESSED;
    return -1;
}

void PACKET_QUEUE_BASE::deduce_module_queue_types()
{
    module_type = get_module_type();
    queue_type = get_queue_type();
}

void PACKET_QUEUE_BASE::record_queue_stats(PACKET *packet)
{
    int8_t module_type = get_module_type();
    int8_t rob_part_type = packet->rob_part_type > 0 ? packet->rob_part_type : 0;
    
    if(packet->enque_cycle[module_type][queue_type] <= 0)
    {
        cout << "=== FAILED ===" << endl;
        cout << "[" << NAME << "] " << " id: " << packet->id << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
        cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
        cout << " instruction: " << (uint32_t)packet->instruction << " is_data: " << (uint32_t)packet->is_data;
        cout << " module_type: " << (int32_t)module_type << " queue_type: " << (int32_t)queue_type;
        cout << " enque_time: " << packet->enque_cycle[module_type][queue_type] << " deque_time: " << packet->deque_cycle[module_type][queue_type] <<  endl;
        assert(false);
    }
    if(packet->deque_cycle[module_type][queue_type] <= 0)
    {
        cout << "=== FAILED ===" << endl;
        cout << "[" << NAME << "] " << " id: " << packet->id << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
        cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
        cout << " instruction: " << (uint32_t)packet->instruction << " is_data: " << (uint32_t)packet->is_data;
        cout << " module_type: " << (int32_t)module_type << " queue_type: " << (int32_t)queue_type;
        cout << " enque_time: " << packet->enque_cycle[module_type][queue_type] << " deque_time: " << packet->deque_cycle[module_type][queue_type]<<  endl;
        assert(false);
    }
    uint64_t delay = packet->deque_cycle[module_type][queue_type] - packet->enque_cycle[module_type][queue_type];
    assert(delay >= 0);

    stats.requests_seen[rob_part_type]++;
    stats.total_queueing_delay[rob_part_type] += delay;
}

void PACKET_QUEUE_BASE::dump_stats()
{
    uint64_t tot_requests_seen = 0, tot_queueing_delay = 0;
    for(uint32_t i = 0; i < NUM_PARTITION_TYPES; ++i)
    {
        tot_requests_seen += stats.requests_seen[i];
        tot_queueing_delay += stats.total_queueing_delay[i];
    }

    cout << "Core_" << cpu << "_" << NAME << "_total_requests " << tot_requests_seen << endl
         << "Core_" << cpu << "_" << NAME << "_total_queueing_delay " << tot_queueing_delay << endl
         << "Core_" << cpu << "_" << NAME << "_avg_queueing_delay " << (float)tot_queueing_delay/tot_requests_seen << endl
         << "Core_" << cpu << "_" << NAME << "_total_requests_FRONTAL " << stats.requests_seen[FRONTAL] << endl
         << "Core_" << cpu << "_" << NAME << "_total_queueing_delay_FRONTAL " << stats.total_queueing_delay[FRONTAL] << endl
         << "Core_" << cpu << "_" << NAME << "_avg_queueing_delay_FRONTAL " << (float)stats.total_queueing_delay[FRONTAL]/stats.requests_seen[FRONTAL] << endl
         << "Core_" << cpu << "_" << NAME << "_total_requests_NONE " << stats.requests_seen[NONE] << endl
         << "Core_" << cpu << "_" << NAME << "_total_queueing_delay_NONE " << stats.total_queueing_delay[NONE] << endl
         << "Core_" << cpu << "_" << NAME << "_avg_queueing_delay_NONE " << (float)stats.total_queueing_delay[NONE]/stats.requests_seen[NONE] << endl
         << "Core_" << cpu << "_" << NAME << "_total_requests_DORSAL " << stats.requests_seen[DORSAL] << endl
         << "Core_" << cpu << "_" << NAME << "_total_queueing_delay_DORSAL " << stats.total_queueing_delay[DORSAL] << endl
         << "Core_" << cpu << "_" << NAME << "_avg_queueing_delay_DORSAL " << (float)stats.total_queueing_delay[DORSAL]/stats.requests_seen[DORSAL] << endl
         << endl;
}

/******************************/
/*    general packet queue    */
/******************************/

int PACKET_QUEUE::check_queue(PACKET *packet, uint64_t timestamp)
{
    if ((head == tail) && occupancy == 0)
        return -1;

    if (head < tail) {
        for (uint32_t i=head; i<tail; i++) {
            if (NAME == "L1D_WQ") {
                if (entry[i].full_addr == packet->full_addr) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
            else {
                if (entry[i].address == packet->address) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
        }
    }
    else {
        for (uint32_t i=head; i<SIZE; i++) {
            if (NAME == "L1D_WQ") {
                if (entry[i].full_addr == packet->full_addr) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
            else {
                if (entry[i].address == packet->address) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
        }
        for (uint32_t i=0; i<tail; i++) {
            if (NAME == "L1D_WQ") {
                if (entry[i].full_addr == packet->full_addr) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
            else {
                if (entry[i].address == packet->address) {
                    DP (if (warmup_complete[packet->cpu]) {
                    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                    cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                    cout << " cycle " << packet->event_cycle << endl; });
                    return i;
                }
            }
        }
    }

    return -1;
}

/* returns the index of the added entry */
int PACKET_QUEUE::add_queue(PACKET *packet, uint64_t timestamp)
{
#ifdef SANITY_CHECK
    if (occupancy && (head == tail))
        assert(0);
#endif
    
    int location = -1;
    
    // add entry
    entry[tail] = *packet;
    location = tail;

    // record enqueue timestamp
    assert(module_type != -1 && queue_type != -1);
    entry[tail].enque_cycle[module_type][queue_type] = timestamp;

    // cout << "[INSIDE_ENQUEUE_" << NAME << "] " << " id: " << packet->id << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
    // cout << " address: " << hex << entry[tail].address << " full_addr: " << entry[tail].full_addr << dec;
    // cout << " instruction: " << (uint32_t)packet->instruction << " is_data: " << (uint32_t)packet->is_data; 
    // cout << " timestamp: " << timestamp << " packet_timestamp: " << packet->enque_cycle[module_type][queue_type] << " entry_timestamp: " << entry[tail].enque_cycle[module_type][queue_type] << endl;

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
    cout << " address: " << hex << entry[tail].address << " full_addr: " << entry[tail].full_addr << dec;
    cout << " head: " << head << " tail: " << tail << " occupancy: " << occupancy << " event_cycle: " << entry[tail].event_cycle << endl; });

    occupancy++;
    tail++;
    if (tail >= SIZE)
        tail = 0;
    
    return location;
}

void PACKET_QUEUE::remove_queue(PACKET *packet, uint64_t timestamp)
{
#ifdef SANITY_CHECK
    if ((occupancy == 0) && (head == tail))
        assert(0);
#endif

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
    cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec << " fill_level: " << packet->fill_level;
    cout << " head: " << head << " tail: " << tail << " occupancy: " << occupancy << " event_cycle: " << packet->event_cycle << endl; });

    // record dequeue timestamp
    assert(module_type != -1 && queue_type != -1);
    packet->deque_cycle[module_type][queue_type] = timestamp;

    // cout << "[INSIDE_DEQUEUE_" << NAME << "] " << " id: " << packet->id << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id;
    // cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
    // cout << " instruction: " << (uint32_t)packet->instruction << " is_data: " << (uint32_t)packet->is_data;
    // cout << " timestamp: " << timestamp << " packet_timestamp: " << packet->deque_cycle[module_type][queue_type] << endl;

    // record queueing stats
    if(is_RQ)
        record_queue_stats(packet);

    // reset entry
    PACKET empty_packet;
    *packet = empty_packet;

    occupancy--;
    head++;
    if (head >= SIZE)
        head = 0;
}

/******************************/
/* priorty-based packet queue */
/******************************/

int PACKET_QUEUE_PRIORITY::check_queue(PACKET *packet, uint64_t timestamp)
{
    assert(entry.size() == occupancy);

    if (occupancy == 0)
        return -1;

    for(uint32_t i = 0; i < entry.size(); ++i)
    {
        if (NAME == "L1D_WQ") 
        {
            if (entry[i].full_addr == packet->full_addr) 
            {
                DP (if (warmup_complete[packet->cpu]) {
                cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                cout << " cycle " << packet->event_cycle << endl; });
                return i;
            }
        }
        else 
        {
            if (entry[i].address == packet->address) 
            {
                DP (if (warmup_complete[packet->cpu]) {
                cout << "[" << NAME << "] " << __func__ << " cpu: " << packet->cpu << " instr_id: " << packet->instr_id << " same address: " << hex << packet->address;
                cout << " full_addr: " << packet->full_addr << dec << " by instr_id: " << entry[i].instr_id << " index: " << i;
                cout << " cycle " << packet->event_cycle << endl; });
                return i;
            }
        }
    }

    return -1;
}

int PACKET_QUEUE_PRIORITY::add_queue(PACKET *packet, uint64_t timestamp)
{
    assert(entry.size() >= occupancy);
    if(occupancy == SIZE)
        assert(0);

    PACKET enque_packet = *packet;

    // record the enqueue timestamp in packet
    assert(module_type != -1 && queue_type != -1);
    enque_packet.enque_cycle[module_type][queue_type] = timestamp;
    
    // if the queue is empty, just add
    if(occupancy == 0)
    {    
        entry.push_back(enque_packet);
        occupancy++;
        head = 0;
        head_iterator = std::prev(entry.end());
        return 0;
    }
    else
    {
        // first add the new packet
        entry.push_back(enque_packet);
        occupancy++;
        
        // update head pointer appropriately
        bool comp = compare_priorty(enque_packet, entry[head]);
        assert(comp != 0); // enforce a strict ordering. There should NOT be two entires with same priority
        if(comp > 0)
        {
            head = (occupancy - 1);
            head_iterator = std::prev(entry.end());
        }

        return (occupancy - 1);
    }
}

void PACKET_QUEUE_PRIORITY::remove_queue(PACKET *packet, uint64_t timestamp)
{
    assert(module_type != -1);

    //sanity check
    assert(entry.size() == occupancy);
    assert(occupancy > 0);
    
    // record the dequeue timestamp in packet
    assert(module_type != -1 && queue_type != -1);
    packet->deque_cycle[module_type][queue_type] = timestamp;

    // record queueing stats
    if(is_RQ)
        record_queue_stats(packet);
    
    // delete the head element
    entry.erase(head_iterator);
    occupancy--;

    // if the queue became empty, head is simply invalid
    if(occupancy == 0)
    {
        head = -1;
        // RBERA_TODO: how to initialize the head_iterator?
        return;
    }

    // if the queue has only one element, set head
    if(occupancy == 1)
    {
        head = 0;
        head_iterator = entry.begin();
        return;
    }

    // otherwise, find the new head
    head = 0;
    head_iterator = entry.begin();
    int32_t ptr = 1;
    for(auto it = entry.begin()+1; it != entry.end(); ++it) // O(n)
    {
        bool comp = compare_priorty(it, head_iterator);
        assert(comp != 0); // enforce a strict ordering. There should NOT be two entires with same priority
        if(comp > 0)
        {
            head = ptr;
            head_iterator = it;
        }
        ptr++;
    }
    return;
}
