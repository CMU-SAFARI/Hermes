#ifndef SHIP_H
#define SHIP_H

#include "cache_repl_base.h"
#include "defs.h"

#define SHIP_maxRRPV 3
#define SHIP_SHCT_SIZE  16384
#define SHCT_PRIME 16381
#define SAMPLER_SET (256*NUM_CPUS)
#define SAMPLER_WAY LLC_WAY
#define SHCT_MAX 7

// sampler structure
class SAMPLER_class
{
  public:
    uint8_t valid,
            type,
            used;

    uint64_t tag, cl_addr, ip;
    
    uint32_t lru;

    SAMPLER_class() 
    {
        valid = 0;
        type = 0;
        used = 0;

        tag = 0;
        cl_addr = 0;
        ip = 0;

        lru = 0;
    };
};


// prediction table structure
class SHCT_class 
{
  public:
    uint32_t counter;

    SHCT_class() {
        counter = 0;
    };
};


class SHiPRepl : public CacheReplBase
{
    public:
        uint32_t rrpv[LLC_SET][LLC_WAY];
        
        // sampler
        uint32_t rand_sets[SAMPLER_SET];
        SAMPLER_class sampler[SAMPLER_SET][SAMPLER_WAY];

        SHCT_class SHCT[NUM_CPUS][SHIP_SHCT_SIZE];

    private:
        uint32_t is_it_sampled(uint32_t set);
        void update_sampler(uint32_t cpu, uint32_t s_idx, uint64_t address, uint64_t ip, uint8_t type);

    public:
        SHiPRepl(string name) : CacheReplBase(name) {}
        ~SHiPRepl(){}
        
        // override fuctions
        void print_config();
        void initialize_replacement();
        void update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit);
        uint32_t find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
        void dump_stats();
};


#endif /* SHIP_H */


