#ifndef HAWKEYE_H
#define HAWKEYE_H

#include <vector>
#include <deque>
#include "cache_repl_base.h"
#include "util.h"

class HawkeyeMetadata
{
    public:
        uint64_t last_touch_pc;
        HawkeyeMetadata() : last_touch_pc(0xdeadbeef) 
        {

        }
        ~HawkeyeMetadata(){}
};

class OPTgen
{
    private:
        uint32_t history_len;
        vector<uint64_t> timestamp;
        vector<deque<pair<uint64_t, uint64_t> > > access_history;
        vector<deque<uint32_t > > occupancy_vector;

    public:
        struct
        {
            struct
            {
                uint64_t called;
                uint64_t access_history_spill;
                uint64_t occupancy_vector_spill;
                uint64_t first_access;
                uint64_t reused_access;
                uint64_t opt_hit;
                uint64_t opt_miss;
            } update;
        } stats;

    public:
        OPTgen();
        ~OPTgen();
        bool update_access(uint64_t address, uint32_t set);
        void dump_stats();
};

class HawkeyePred
{
    private:
        Counter** confidence;

    public:
        struct
        {
            struct
            {
                uint64_t called;
                uint64_t incr;
                uint64_t decr;
            } train;

            struct
            {
                uint64_t called;
                uint64_t cache_friendly;
                uint64_t cache_adverse;
            } predict;
        } stats;

    private:
        uint32_t gen_index(uint64_t ip);

    public:
        HawkeyePred();
        ~HawkeyePred();
        void train(uint64_t ip, bool opt_decision);
        bool predict(uint64_t ip);
        void dump_stats();
};

class HawkeyeRepl : public CacheReplBase
{
    public:
        OPTgen *optgen;
        HawkeyePred *predictor;

        // metada per cacheline
        uint32_t **rrip;
        HawkeyeMetadata ***metadata;

        struct
        {
            struct
            {
                uint64_t called;
                uint64_t cache_friendly;
                uint64_t cache_friendly_hit;
                uint64_t cache_friendly_miss;
                uint64_t cache_adverse;
                uint64_t cache_adverse_hit;
                uint64_t cache_adverse_miss;
            } update_repl_state;

            struct
            {
                uint64_t called;
                uint64_t max_rrip_found;
                uint64_t max_rrip_not_found;
            } find_victim;
        } stats;

    private:
        void init_knobs();
        void init_stats();

    public:
        HawkeyeRepl(string name);
        ~HawkeyeRepl();

        // override fuctions
        void print_config();
        void initialize_replacement();
        void update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit);
        uint32_t find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
        void dump_stats();
};



#endif /* HAWKEYE_H */

