#ifndef CACHE_REPL_BASE_H
#define CACHE_REPL_BASE_H

#include <string>
#include "block.h"

using namespace std;

class CacheReplBase
{
    protected:
        string name;

    public:
        CacheReplBase(string _name) : name (_name) {}
        ~CacheReplBase(){}

        virtual void print_config() = 0;
        virtual void initialize_replacement() = 0;
        virtual uint32_t find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type) = 0;
        virtual void update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit) = 0;
        virtual void dump_stats() = 0;
};

#endif /* CACHE_REPL_BASE_H */


