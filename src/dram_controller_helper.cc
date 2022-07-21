#include "dram_controller.h"

int MEMORY_CONTROLLER::schedule_FR_FCFS(PACKET_QUEUE *queue, uint8_t& row_buffer_hit)
{
    int index = -1;
    uint64_t oldest_cycle = UINT64_MAX, read_addr;
    uint32_t read_channel, read_rank, read_bank, read_row;

    // first, search for the oldest open row hit
    for (uint32_t i=0; i<queue->SIZE; i++) 
    {
        // already scheduled
        if (queue->entry[i].scheduled) 
            continue;

        // empty entry
        read_addr = queue->entry[i].address;
        if (read_addr == 0) 
            continue;

        read_channel = dram_get_channel(read_addr);
        read_rank = dram_get_rank(read_addr);
        read_bank = dram_get_bank(read_addr);

        // bank is busy
        if (bank_request[read_channel][read_rank][read_bank].working)
            continue;

        read_row = dram_get_row(read_addr);

        if (bank_request[read_channel][read_rank][read_bank].open_row != read_row) 
            continue;

        // select the oldest entry
        if (queue->entry[i].event_cycle < oldest_cycle) 
        {
            oldest_cycle = queue->entry[i].event_cycle;
            index = i;
            row_buffer_hit = 1;
        }	  
    }

    // if no matching open_row (row buffer miss)
    if (index == -1) 
    { 
        oldest_cycle = UINT64_MAX;
        for (uint32_t i=0; i<queue->SIZE; i++)
        {
            // already scheduled
            if (queue->entry[i].scheduled)
                continue;

            // empty entry
            read_addr = queue->entry[i].address;
            if (read_addr == 0) 
                continue;

            read_channel = dram_get_channel(read_addr);
            read_rank = dram_get_rank(read_addr);
            read_bank = dram_get_bank(read_addr);
            
            // bank is busy
            if (bank_request[read_channel][read_rank][read_bank].working) 
                continue;

            // select the oldest entry
            if (queue->entry[i].event_cycle < oldest_cycle) 
            {
                oldest_cycle = queue->entry[i].event_cycle;
                index = i;
            }
        }
    }

    return index;
}

int MEMORY_CONTROLLER::schedule_CR_FCFS(PACKET_QUEUE *queue, uint8_t& row_buffer_hit)
{
    int index = -1;
    uint64_t oldest_cycle = UINT64_MAX, read_addr;
    uint32_t read_channel, read_rank, read_bank, read_row;

    // first, search for the oldest row miss
    for (uint32_t i=0; i<queue->SIZE; i++) 
    {
        // already scheduled
        if (queue->entry[i].scheduled) 
            continue;

        // empty entry
        read_addr = queue->entry[i].address;
        if (read_addr == 0) 
            continue;

        read_channel = dram_get_channel(read_addr);
        read_rank = dram_get_rank(read_addr);
        read_bank = dram_get_bank(read_addr);

        // bank is busy
        if (bank_request[read_channel][read_rank][read_bank].working)
            continue;

        read_row = dram_get_row(read_addr);

        // only look for row buffer miss requests
        if (bank_request[read_channel][read_rank][read_bank].open_row == read_row) 
            continue;

        // select the oldest entry
        if (queue->entry[i].event_cycle < oldest_cycle) 
        {
            oldest_cycle = queue->entry[i].event_cycle;
            index = i;
            row_buffer_hit = 0;
        }	  
    }

    // if no closed row (all row buffer hits)
    if (index == -1) 
    { 
        row_buffer_hit = 1;
        oldest_cycle = UINT64_MAX;
        for (uint32_t i=0; i<queue->SIZE; i++)
        {
            // already scheduled
            if (queue->entry[i].scheduled)
                continue;

            // empty entry
            read_addr = queue->entry[i].address;
            if (read_addr == 0) 
                continue;

            read_channel = dram_get_channel(read_addr);
            read_rank = dram_get_rank(read_addr);
            read_bank = dram_get_bank(read_addr);
            
            // bank is busy
            if (bank_request[read_channel][read_rank][read_bank].working) 
                continue;

            // select the oldest entry
            if (queue->entry[i].event_cycle < oldest_cycle) 
            {
                oldest_cycle = queue->entry[i].event_cycle;
                index = i;
            }
        }
    }

    return index;
}

int MEMORY_CONTROLLER::schedule_FRONTAL_FR_FCFS(PACKET_QUEUE *queue, uint8_t& row_buffer_hit)
{
    int sel_index = -1;
    bool sel_is_frontal = false;
    bool sel_row_buffer_hit = false;
    uint64_t sel_cycle = UINT64_MAX;

    int read_index;
    bool read_is_frontal;
    bool read_row_buffer_hit;
    uint64_t read_cycle;
    
    uint64_t read_addr;
    uint32_t read_channel, read_rank, read_bank, read_row;
    bool update = false;

    for (uint32_t i=0; i<queue->SIZE; i++) 
    {
        // already scheduled
        if (queue->entry[i].scheduled) 
            continue;

        // empty entry
        read_addr = queue->entry[i].address;
        if (read_addr == 0) 
            continue;

        read_channel = dram_get_channel(read_addr);
        read_rank = dram_get_rank(read_addr);
        read_bank = dram_get_bank(read_addr);

        // bank is busy
        if (bank_request[read_channel][read_rank][read_bank].working)
            continue;

        read_row = dram_get_row(read_addr);

        read_index = i;
        read_is_frontal = (queue->entry[i].rob_part_type <= FRONTAL);
        read_row_buffer_hit = (bank_request[read_channel][read_rank][read_bank].open_row == read_row);
        read_cycle = queue->entry[i].event_cycle;

        update = false;

        if(read_is_frontal && !sel_is_frontal)
        {
            update = true;
        }
        else if(!read_is_frontal && sel_is_frontal)
        {
            // no need to update
        }
        else // either both frontal or both non-frontal
        {
            if(read_row_buffer_hit && !sel_row_buffer_hit)
            {
                update = true;
            }
            else if(!read_row_buffer_hit && sel_row_buffer_hit)
            {
                // no need to update
            }
            else // either both row buffer hits or both row buffer miss
            {
                if(read_cycle < sel_cycle)
                {
                    update = true;
                }
                else
                {
                    // no need to update
                }
            }
        }

        if(update)
        {
            sel_index = read_index;
            sel_is_frontal = read_is_frontal;
            sel_row_buffer_hit = read_row_buffer_hit;
            sel_cycle = read_cycle;
        }
    }

    row_buffer_hit = sel_row_buffer_hit;
    return sel_index;
}

int MEMORY_CONTROLLER::schedule_FR_FRONTAL_FCFS(PACKET_QUEUE *queue, uint8_t& row_buffer_hit)
{
    int sel_index = -1;
    bool sel_is_frontal = false;
    bool sel_row_buffer_hit = false;
    uint64_t sel_cycle = UINT64_MAX;

    int read_index;
    bool read_is_frontal;
    bool read_row_buffer_hit;
    uint64_t read_cycle;
    
    uint64_t read_addr;
    uint32_t read_channel, read_rank, read_bank, read_row;
    bool update = false;

    for (uint32_t i=0; i<queue->SIZE; i++) 
    {
        // already scheduled
        if (queue->entry[i].scheduled) 
            continue;

        // empty entry
        read_addr = queue->entry[i].address;
        if (read_addr == 0) 
            continue;

        read_channel = dram_get_channel(read_addr);
        read_rank = dram_get_rank(read_addr);
        read_bank = dram_get_bank(read_addr);

        // bank is busy
        if (bank_request[read_channel][read_rank][read_bank].working)
            continue;

        read_row = dram_get_row(read_addr);

        read_index = i;
        read_is_frontal = (queue->entry[i].rob_part_type <= FRONTAL);
        read_row_buffer_hit = (bank_request[read_channel][read_rank][read_bank].open_row == read_row);
        read_cycle = queue->entry[i].event_cycle;

        update = false;

        if(read_row_buffer_hit && !sel_row_buffer_hit)
        {
            update = true;
        }
        else if(!read_row_buffer_hit && sel_row_buffer_hit)
        {
            // no need to update
        }
        else // either both row buffer hits or both row buffer miss
        {
            if(read_is_frontal && !sel_is_frontal)
            {
                update = true;
            }
            else if(!read_is_frontal && sel_is_frontal)
            {
                // no need to update
            }
            else // either both frontal or both non-frontal
            {
                if(read_cycle < sel_cycle)
                {
                    update = true;
                }
                else
                {
                    // no need to update
                }
            }
        }

        if(update)
        {
            sel_index = read_index;
            sel_is_frontal = read_is_frontal;
            sel_row_buffer_hit = read_row_buffer_hit;
            sel_cycle = read_cycle;
        }
    }
    
    row_buffer_hit = sel_row_buffer_hit;
    return sel_index;
}

int MEMORY_CONTROLLER::schedule_ROB_PART_FR_FCFS(PACKET_QUEUE *queue, uint8_t& row_buffer_hit)
{
    int sel_index = -1;
    int8_t sel_rob_part = INT8_MAX;
    bool sel_row_buffer_hit = false;
    uint64_t sel_cycle = UINT64_MAX;

    int read_index;
    bool read_rob_part;
    bool read_row_buffer_hit;
    uint64_t read_cycle;
    
    uint64_t read_addr;
    uint32_t read_channel, read_rank, read_bank, read_row;
    bool update = false;

    for (uint32_t i=0; i<queue->SIZE; i++) 
    {
        // already scheduled
        if (queue->entry[i].scheduled) 
            continue;

        // empty entry
        read_addr = queue->entry[i].address;
        if (read_addr == 0) 
            continue;

        read_channel = dram_get_channel(read_addr);
        read_rank = dram_get_rank(read_addr);
        read_bank = dram_get_bank(read_addr);

        // bank is busy
        if (bank_request[read_channel][read_rank][read_bank].working)
            continue;

        read_row = dram_get_row(read_addr);

        read_index = i;
        read_rob_part = queue->entry[i].rob_part_type;
        read_row_buffer_hit = (bank_request[read_channel][read_rank][read_bank].open_row == read_row);
        read_cycle = queue->entry[i].event_cycle;

        update = false;

        if(read_rob_part < sel_rob_part)
        {
            update = true;
        }
        else if(read_rob_part > sel_rob_part)
        {
            // no need to update
        }
        else // both loads are from same ROB partitions
        {
            if(read_row_buffer_hit && !sel_row_buffer_hit)
            {
                update = true;
            }
            else if(!read_row_buffer_hit && sel_row_buffer_hit)
            {
                // no need to update
            }
            else // either both row buffer hits or both row buffer miss
            {
                if(read_cycle < sel_cycle)
                {
                    update = true;
                }
                else
                {
                    // no need to update
                }
            }
        }

        if(update)
        {
            sel_index = read_index;
            sel_rob_part = read_rob_part;
            sel_row_buffer_hit = read_row_buffer_hit;
            sel_cycle = read_cycle;
        }
    }

    row_buffer_hit = sel_row_buffer_hit;
    return sel_index;
}

int MEMORY_CONTROLLER::schedule_FR_ROB_PART_FCFS(PACKET_QUEUE *queue, uint8_t& row_buffer_hit)
{
    int sel_index = -1;
    int8_t sel_rob_part = INT8_MAX;
    bool sel_row_buffer_hit = false;
    uint64_t sel_cycle = UINT64_MAX;

    int read_index;
    bool read_rob_part;
    bool read_row_buffer_hit;
    uint64_t read_cycle;
    
    uint64_t read_addr;
    uint32_t read_channel, read_rank, read_bank, read_row;
    bool update = false;

    for (uint32_t i=0; i<queue->SIZE; i++) 
    {
        // already scheduled
        if (queue->entry[i].scheduled) 
            continue;

        // empty entry
        read_addr = queue->entry[i].address;
        if (read_addr == 0) 
            continue;

        read_channel = dram_get_channel(read_addr);
        read_rank = dram_get_rank(read_addr);
        read_bank = dram_get_bank(read_addr);

        // bank is busy
        if (bank_request[read_channel][read_rank][read_bank].working)
            continue;

        read_row = dram_get_row(read_addr);

        read_index = i;
        read_rob_part = queue->entry[i].rob_part_type;
        read_row_buffer_hit = (bank_request[read_channel][read_rank][read_bank].open_row == read_row);
        read_cycle = queue->entry[i].event_cycle;

        update = false;

        if(read_row_buffer_hit && !sel_row_buffer_hit)
        {
            update = true;
        }
        else if(!read_row_buffer_hit && sel_row_buffer_hit)
        {
            // no need to update
        }
        else // either both row buffer hits or both row buffer miss
        {
            if(read_rob_part < sel_rob_part)
            {
                update = true;
            }
            else if(read_rob_part > sel_rob_part)
            {
                // no need to update
            }
            else // both loads are from same ROB partitions 
            {
                if(read_cycle < sel_cycle)
                {
                    update = true;
                }
                else
                {
                    // no need to update
                }
            }
        }

        if(update)
        {
            sel_index = read_index;
            sel_rob_part = read_rob_part;
            sel_row_buffer_hit = read_row_buffer_hit;
            sel_cycle = read_cycle;
        }
    }

    row_buffer_hit = sel_row_buffer_hit;
    return sel_index;
}