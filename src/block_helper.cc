#include "block.h"

string priority_name_string[] = {
    "FCFS",
    "ROB_POS_FCFS",
    "OFFCHIP_FCFS",
    "OFFCHIP_ROB_POS_FCFS"
};

int32_t PACKET_QUEUE_PRIORITY::priority_FCFS(PACKET packet1, PACKET packet2)
{
    if(packet1.enque_cycle[module_type][queue_type] < packet2.enque_cycle[module_type][queue_type])         return 1;
    else if(packet2.enque_cycle[module_type][queue_type] < packet1.enque_cycle[module_type][queue_type])    return -1;
    else
    {
        if(packet1.id < packet2.id)         return 1;
        else if(packet2.id < packet1.id)    return -1;
    }
    return 0;
}

int32_t PACKET_QUEUE_PRIORITY::priority_ROB_POS_FCFS(PACKET packet1, PACKET packet2)
{
    int8_t rob_part_type1 = packet1.rob_part_type >= 0 ? packet1.rob_part_type : 0;
    int8_t rob_part_type2 = packet2.rob_part_type >= 0 ? packet2.rob_part_type : 0;

    if(rob_part_type1 < rob_part_type2)         return 1;
    else if (rob_part_type2 < rob_part_type1)   return -1;
    else                                        return priority_FCFS(packet1, packet2);
}

int32_t PACKET_QUEUE_PRIORITY::priority_OFFCHIP_FCFS(PACKET packet1, PACKET packet2)
{
    if(packet1.went_offchip_pred == 1 && packet2.went_offchip_pred == 0)         return 1;
    else if (packet2.went_offchip_pred == 1 && packet1.went_offchip_pred == 0)   return -1;
    else                                                                         return priority_FCFS(packet1, packet2);
}

int32_t PACKET_QUEUE_PRIORITY::priority_OFFCHIP_ROB_POS_FCFS(PACKET packet1, PACKET packet2)
{
    if(packet1.went_offchip_pred == 1 && packet2.went_offchip_pred == 0)         return 1;
    else if (packet2.went_offchip_pred == 1 && packet1.went_offchip_pred == 0)   return -1;
    else                                                                         return priority_ROB_POS_FCFS(packet1, packet2);
}

int32_t PACKET_QUEUE_PRIORITY::compare_priorty(PACKET packet1, PACKET packet2)
{
    switch(priority_type)
    {
        case FCFS:                      return priority_FCFS(packet1, packet2);
        case ROB_POS_FCFS:              return priority_ROB_POS_FCFS(packet1, packet2);
        case OFFCHIP_FCFS:              return priority_OFFCHIP_FCFS(packet1, packet2);
        case OFFCHIP_ROB_POS_FCFS:      return priority_OFFCHIP_ROB_POS_FCFS(packet1, packet2);
        default:                        return 0;
    }
}

int32_t PACKET_QUEUE_PRIORITY::compare_priorty(deque<PACKET>::iterator it1, deque<PACKET>::iterator it2)
{
    PACKET packet1 = (*it1), packet2 = (*it2);
    return compare_priorty(packet1, packet2);
}