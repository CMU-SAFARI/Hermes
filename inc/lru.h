#ifndef LRU_H
#define LRU_H

#include "cache.h"
#include "cache_repl_base.h"

class LRURepl : public CacheReplBase
{
    public:
        CACHE *parent;

    public:
        LRURepl(string name, CACHE *cache) : CacheReplBase(name), parent(cache) {}
        ~LRURepl(){}

        // override fuctions
        void print_config();
        void initialize_replacement();
        void update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit);
        uint32_t find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
        void dump_stats();
};

#endif /* LRU_H */

