//Hawkeye Cache Replacement Tool v2.0
//UT AUSTIN RESEARCH LICENSE (SOURCE CODE)
//The University of Texas at Austin has developed certain software and documentation that it desires to
//make available without charge to anyone for academic, research, experimental or personal use.
//This license is designed to guarantee freedom to use the software for these purposes. If you wish to
//distribute or make other use of the software, you may purchase a license to do so from the University of
//Texas.
////////////////////////////////////////////////
//                                            //
//     Hawkeye [Jain and Lin, ISCA' 16]       //
//     Akanksha Jain, akanksha@cs.utexas.edu  //
//                                            //
////////////////////////////////////////////////

#ifndef HAWKEYE_CRC2_H
#define HAWKEYE_CRC2_H

#include <iostream>
#include <math.h>
#include <set>
#include <vector>
#include <map>
#include <cassert>
#include "defs.h"
#include "cache_repl_base.h"
#include "util.h"
using namespace std;


/* necessary variables for Hawkeye */
//3-bit RRIP counters or all lines
#define maxRRPV 7
//Per-set timers; we only use 64 of these
//Budget = 64 sets * 1 timer per set * 10 bits per timer = 80 bytes
#define TIMER_SIZE 1024
// Hawkeye Predictors for demand and prefetch requests
// Predictor with 2K entries and 5-bit counter per entry
// Budget = 2048*5/8 bytes = 1.2KB
#define MAX_SHCT 31
#define SHCT_SIZE_BITS 11
#define SHCT_SIZE (1<<SHCT_SIZE_BITS)
#define OPTGEN_VECTOR_SIZE 128
#define bitmask(l) (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l))-1LL))
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))
//Sample 64 sets per core
#define SAMPLED_SET(set) (bits(set, 0 , 6) == bits(set, ((unsigned long long)log2(LLC_SET) - 6), 6) )

// Sampler to track 8x cache history for sampled sets
// 2800 entris * 4 bytes per entry = 11.2KB
#define SAMPLED_CACHE_SIZE 2800
#define SAMPLER_WAYS 8
#define SAMPLER_SETS SAMPLED_CACHE_SIZE/SAMPLER_WAYS


/* moved this hash function to util.h */
// uint64_t HashZoo::crc64( uint64_t _blockAddress )
// {
//     static const unsigned long long crcPolynomial = 3988292384ULL;
//     unsigned long long _returnVal = _blockAddress;
//     for( unsigned int i = 0; i < 32; i++ )
//         _returnVal = ( ( _returnVal & 1 ) == 1 ) ? ( ( _returnVal >> 1 ) ^ crcPolynomial ) : ( _returnVal >> 1 );
//     return _returnVal;
// }

class HAWKEYE_PC_PREDICTOR
{
    map<uint64_t, short unsigned int > SHCT;

       public:

    void increment (uint64_t pc)
    {
        uint64_t signature = HashZoo::crc64(pc) % SHCT_SIZE;
        if(SHCT.find(signature) == SHCT.end())
            SHCT[signature] = (1+MAX_SHCT)/2;

        SHCT[signature] = (SHCT[signature] < MAX_SHCT) ? (SHCT[signature]+1) : MAX_SHCT;

    }

    void decrement (uint64_t pc)
    {
        uint64_t signature = HashZoo::crc64(pc) % SHCT_SIZE;
        if(SHCT.find(signature) == SHCT.end())
            SHCT[signature] = (1+MAX_SHCT)/2;
        if(SHCT[signature] != 0)
            SHCT[signature] = SHCT[signature]-1;
    }

    bool get_prediction (uint64_t pc)
    {
        uint64_t signature = HashZoo::crc64(pc) % SHCT_SIZE;
        if(SHCT.find(signature) != SHCT.end() && SHCT[signature] < ((MAX_SHCT+1)/2))
            return false;
        return true;
    }
};

struct ADDR_INFO
{
    uint64_t addr;
    uint32_t last_quanta;
    uint64_t PC; 
    bool prefetched;
    uint32_t lru;

    void init(unsigned int curr_quanta)
    {
        last_quanta = 0;
        PC = 0;
        prefetched = false;
        lru = 0;
    }

    void update(unsigned int curr_quanta, uint64_t _pc, bool prediction)
    {
        last_quanta = curr_quanta;
        PC = _pc;
    }

    void mark_prefetch()
    {
        prefetched = true;
    }
};

struct OPTGen
{
    vector<unsigned int> liveness_history;

    uint64_t num_cache;
    uint64_t num_dont_cache;
    uint64_t access;

    uint64_t CACHE_SIZE;

    void init(uint64_t size)
    {
        num_cache = 0;
        num_dont_cache = 0;
        access = 0;
        CACHE_SIZE = size;
        liveness_history.resize(OPTGEN_VECTOR_SIZE, 0);
    }

    void add_access(uint64_t curr_quanta)
    {
        access++;
        liveness_history[curr_quanta] = 0;
    }

    void add_prefetch(uint64_t curr_quanta)
    {
        liveness_history[curr_quanta] = 0;
    }

    bool should_cache(uint64_t curr_quanta, uint64_t last_quanta)
    {
        bool is_cache = true;

        unsigned int i = last_quanta;
        while (i != curr_quanta)
        {
            if(liveness_history[i] >= CACHE_SIZE)
            {
                is_cache = false;
                break;
            }

            i = (i+1) % liveness_history.size();
        }


        //if ((is_cache) && (last_quanta != curr_quanta))
        if ((is_cache))
        {
            i = last_quanta;
            while (i != curr_quanta)
            {
                liveness_history[i]++;
                i = (i+1) % liveness_history.size();
            }
            assert(i == curr_quanta);
        }

        if (is_cache) num_cache++;
        else num_dont_cache++;

        return is_cache;    
    }

    uint64_t get_num_opt_hits()
    {
        return num_cache;

        uint64_t num_opt_misses = access - num_cache;
        return num_opt_misses;
    }
};

class HawkeyeCRC2Repl : public CacheReplBase
{
    private:
        uint32_t rrpv[LLC_SET][LLC_WAY];
        uint64_t perset_mytimer[LLC_SET];
        
        // Signatures for sampled sets; we only use 64 of these
        // Budget = 64 sets * 16 ways * 12-bit signature per line = 1.5B
        uint64_t signatures[LLC_SET][LLC_WAY];
        bool prefetched[LLC_SET][LLC_WAY];
        
        HAWKEYE_PC_PREDICTOR* demand_predictor;  //Predictor
        HAWKEYE_PC_PREDICTOR* prefetch_predictor;  //Predictor
        
        OPTGen perset_optgen[LLC_SET]; // per-set occupancy vectors; we only use 64 of these
        vector<map<uint64_t, ADDR_INFO> > addr_history; // Sampler
    
    private:
        void replace_addr_history_element(unsigned int sampler_set);
        void update_addr_history_lru(unsigned int sampler_set, unsigned int curr_lru);

    public:
        HawkeyeCRC2Repl(string name) : CacheReplBase(name) {}
        ~HawkeyeCRC2Repl() {}

        // override fuctions
        void print_config();
        void initialize_replacement();
        void update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit);
        uint32_t find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
        void dump_stats();
};

#endif /* HAWKEYE_CRC2_H */


