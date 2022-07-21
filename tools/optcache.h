#ifndef OPTCACHE_H
#define OPTCACHE_H

#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <vector>
#include <deque>

using namespace std;

#define LOG2_BLOCK_SIZE 6

// CACHE ACCESS TYPE
#define LOAD      0
#define RFO       1
#define PREFETCH  2
#define WRITEBACK 3
#define NUM_TYPES 4

class OptCacheBlock
{
    public:
        uint64_t tag;
        uint64_t reuse_dist;
        bool valid;

    public:
        OptCacheBlock()
        {
            tag = 0xdeadbeef;
            reuse_dist = 0;
            valid = false;
        }
        ~OptCacheBlock(){}
};

class OptCache
{
    private:
        uint32_t num_sets, num_assoc;
        bool bypass_en;
        OptCacheBlock*** blocks;

        struct
        {
            struct
            {
                uint64_t total[NUM_TYPES];
                uint64_t hit[NUM_TYPES];
                uint64_t miss[NUM_TYPES];
            } access;

            struct
            {
                uint64_t total[NUM_TYPES];
                uint64_t hit[NUM_TYPES];
                uint64_t miss[NUM_TYPES];
            } trace;

            struct
            {
                uint64_t insertion;
                uint64_t eviction;
                uint64_t bypass;
                uint64_t promotion;
            } cache;
            
        } stats;

    private:
        uint32_t get_set(uint64_t address);
        int32_t lookup_set(uint64_t ca_address, uint32_t set);
        int32_t find_victim(uint32_t set, uint64_t &victim_reuse_dist);
    
    public:
        OptCache(uint32_t sets, uint32_t assoc, bool bypass_en);
        ~OptCache();
        void access(uint64_t addr, uint8_t type, bool hit, uint64_t reuse_dist);
        void dump_stats();
};

OptCache::OptCache(uint32_t _sets, uint32_t _assoc, bool _bypass_en)
{
    num_sets = _sets;
    num_assoc = _assoc;
    bypass_en = _bypass_en;

    bzero(&stats, sizeof(stats));

    // init cache
    blocks = (OptCacheBlock***)calloc(num_sets, sizeof(OptCacheBlock**));
    assert(blocks);
    for(uint32_t set = 0; set < num_sets; ++set)
    {
        blocks[set] = (OptCacheBlock**)calloc(num_assoc, sizeof(OptCacheBlock*));
        assert(blocks[set]);
        for(uint32_t way = 0; way < num_assoc; ++way)
        {
            blocks[set][way] = new OptCacheBlock();
        }
    }
}

OptCache::~OptCache()
{
    for(uint32_t set = 0; set < num_sets; ++set)
    {
        for(uint32_t way = 0; way < num_assoc; ++way)
        {
            free(blocks[set][way]);
        }
        free(blocks[set]);
    }
}

void OptCache::access(uint64_t address, uint8_t type, bool hit, uint64_t reuse_dist)
{
    uint64_t ca_address = (address >> LOG2_BLOCK_SIZE) << LOG2_BLOCK_SIZE;
 
    // capture trace stats
    stats.trace.total[type]++;
    if(hit) stats.trace.hit[type]++; else stats.trace.miss[type]++;

    uint32_t set = get_set(ca_address);
    int32_t way = lookup_set(ca_address, set);
    
    if(way != -1) // cache hit
    {
        blocks[set][way]->reuse_dist = reuse_dist;
        stats.cache.promotion++;
        stats.access.hit[type]++;
        stats.access.total[type]++;
    }
    else // cache miss
    {
        uint64_t victim_reuse_dist = 0;
        way = find_victim(set, victim_reuse_dist);
        if(bypass_en 
            && blocks[set][way]->valid 
            && reuse_dist > victim_reuse_dist 
            && type != 3) // all other types of accesses can be bypassed, except WRITEBACKs
        {
            // bypass
            stats.cache.bypass++;
        }
        else
        {
            // eviction
            if(blocks[set][way]->valid)
            {
                stats.cache.eviction++;
            }

            // insertion
            blocks[set][way]->tag = ca_address;
            blocks[set][way]->reuse_dist = reuse_dist;
            blocks[set][way]->valid = true;
            stats.cache.insertion++;
        }

        // update stats
        stats.access.miss[type]++;
        stats.access.total[type]++;
    }
}

uint32_t OptCache::get_set(uint64_t address)
{
    uint64_t addr = (address >> LOG2_BLOCK_SIZE);
    return (addr % num_sets);
}

int32_t OptCache::lookup_set(uint64_t ca_address, uint32_t set)
{
    for(uint32_t way = 0; way < num_assoc; ++way)
    {
        if(blocks[set][way]->valid && blocks[set][way]->tag == ca_address)
        {
            return way;
        }
    }

    return -1;
}

int32_t OptCache::find_victim(uint32_t set, uint64_t &victim_reuse_dist)
{
    uint64_t max_reuse_dist = 0;
    int32_t victim_way = -1;

    for(uint32_t way = 0; way < num_assoc; ++way)
    {
        if(!blocks[set][way]->valid)
        {
            victim_reuse_dist = 0;
            return way;
        }

        if(blocks[set][way]->reuse_dist >= max_reuse_dist)
        {
            victim_way = way;
            max_reuse_dist = blocks[set][way]->reuse_dist;
        }
    }

    assert(victim_way != -1);
    victim_reuse_dist = max_reuse_dist;
    return victim_way;
}

void OptCache::dump_stats()
{
    uint64_t access_total = 0, access_hit = 0, access_miss = 0;
    for(uint32_t type = 0; type < NUM_TYPES; ++type)
    {
        access_total += stats.access.total[type];
        access_hit += stats.access.hit[type];
        access_miss += stats.access.miss[type];
    }
    uint64_t trace_total = 0, trace_hit = 0, trace_miss = 0;
    for(uint32_t type = 0; type < NUM_TYPES; ++type)
    {
        trace_total += stats.trace.total[type];
        trace_hit += stats.trace.hit[type];
        trace_miss += stats.trace.miss[type];
    }

    cout << "OptCache.access.total " << access_total << endl
        << "OptCache.access.hit " << access_hit << endl
        << "OptCache.access.miss " << access_miss << endl
        << "OptCache.access.LOAD.total " << stats.access.total[0] << endl
        << "OptCache.access.LOAD.hit " << stats.access.hit[0] << endl
        << "OptCache.access.LOAD.miss " << stats.access.miss[0] << endl
        << "OptCache.access.RFO.total " << stats.access.total[1] << endl
        << "OptCache.access.RFO.hit " << stats.access.hit[1] << endl
        << "OptCache.access.RFO.miss " << stats.access.miss[1] << endl
        << "OptCache.access.PREFETCH.total " << stats.access.total[2] << endl
        << "OptCache.access.PREFETCH.hit " << stats.access.hit[2] << endl
        << "OptCache.access.PREFETCH.miss " << stats.access.miss[2] << endl
        << "OptCache.access.WRITEBACK.total " << stats.access.total[3] << endl
        << "OptCache.access.WRITEBACK.hit " << stats.access.hit[3] << endl
        << "OptCache.access.WRITEBACK.miss " << stats.access.miss[3] << endl
        << endl
        << "OptCache.cache.insertion " << stats.cache.insertion << endl
        << "OptCache.cache.eviction " << stats.cache.eviction << endl
        << "OptCache.cache.bypass " << stats.cache.bypass << endl
        << "OptCache.cache.promotion " << stats.cache.promotion << endl
        << endl
        << "OptCache.trace.total " << trace_total << endl
        << "OptCache.trace.hit " << trace_hit << endl
        << "OptCache.trace.miss " << trace_miss << endl
        << "OptCache.trace.LOAD.total " << stats.trace.total[0] << endl
        << "OptCache.trace.LOAD.hit " << stats.trace.hit[0] << endl
        << "OptCache.trace.LOAD.miss " << stats.trace.miss[0] << endl
        << "OptCache.trace.RFO.total " << stats.trace.total[1] << endl
        << "OptCache.trace.RFO.hit " << stats.trace.hit[1] << endl
        << "OptCache.trace.RFO.miss " << stats.trace.miss[1] << endl
        << "OptCache.trace.PREFETCH.total " << stats.trace.total[2] << endl
        << "OptCache.trace.PREFETCH.hit " << stats.trace.hit[2] << endl
        << "OptCache.trace.PREFETCH.miss " << stats.trace.miss[2] << endl
        << "OptCache.trace.WRITEBACK.total " << stats.trace.total[3] << endl
        << "OptCache.trace.WRITEBACK.hit " << stats.trace.hit[3] << endl
        << "OptCache.trace.WRITEBACK.miss " << stats.trace.miss[3] << endl
        << endl;
}




#endif /* OPTCACHE_H */


