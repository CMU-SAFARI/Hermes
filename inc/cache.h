#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <random>
#include "memory_class.h"
#include "prefetcher.h"
#include "defs.h"
#include "cache_repl_base.h"

// PAGE
extern uint32_t PAGE_TABLE_LATENCY, SWAP_LATENCY;

class CACHE : public MEMORY {
  public:
    uint32_t cpu;
    const string NAME;
    const uint32_t NUM_SET, NUM_WAY, NUM_LINE, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
    uint32_t LATENCY;
    BLOCK **block;
    int fill_level;
    uint32_t MAX_READ, MAX_FILL;
    uint32_t reads_available_this_cycle;
    uint8_t cache_type;

    // prefetch stats
    uint64_t pf_requested,
             pf_issued,
             pf_useful,
             pf_useless,
             pf_dropped,
             pf_filled,
             pf_late;

    // queues
    PACKET_QUEUE WQ{NAME + "_WQ", WQ_SIZE}, // write queue
                //  RQ{NAME + "_RQ", RQ_SIZE}, // read queue
                 PQ{NAME + "_PQ", PQ_SIZE}, // prefetch queue
                 MSHR{NAME + "_MSHR", MSHR_SIZE}, // MSHR
                 PROCESSED{NAME + "_PROCESSED", ROB_SIZE}; // processed queue

    // PACKET_QUEUE_PRIORITY RQ{NAME + "_RQ", RQ_SIZE}; // read queue
    
    // initialized in constructor
    PACKET_QUEUE_BASE *RQ = NULL;

    uint64_t sim_access[NUM_CPUS][NUM_TYPES],
             sim_hit[NUM_CPUS][NUM_TYPES],
             sim_miss[NUM_CPUS][NUM_TYPES],
             roi_access[NUM_CPUS][NUM_TYPES],
             roi_hit[NUM_CPUS][NUM_TYPES],
             roi_miss[NUM_CPUS][NUM_TYPES];

    // some stats to collect during cache eviction
    struct
    {
        struct
        {
            uint64_t total;

            uint64_t atleast_one_reuse;
            uint64_t atleast_one_load_reuse;
            uint64_t atleast_one_reuse_cat[NUM_TYPES];

            uint64_t all_reuse_total;
            uint64_t all_reuse_max;
            uint64_t all_reuse_min;

            uint64_t cat_reuse_total[NUM_TYPES];
            uint64_t cat_reuse_max[NUM_TYPES];
            uint64_t cat_reuse_min[NUM_TYPES];

            uint64_t reuse_only_frontal;
            uint64_t reuse_only_dorsal;
            uint64_t reuse_only_none;
            uint64_t reuse_mixed;

            // all dependents
            uint64_t dep_all_total;
            uint64_t dep_all_max;
            uint64_t dep_all_min;
            // mispredicted branch dependents
            uint64_t dep_branch_mispred_total;
            uint64_t dep_branch_mispred_max;
            uint64_t dep_branch_mispred_min;
            // all branch dependents
            uint64_t dep_branch_total;
            uint64_t dep_branch_max;
            uint64_t dep_branch_min;
            // load dependents
            uint64_t dep_load_total;
            uint64_t dep_load_max;
            uint64_t dep_load_min;
        } eviction;

        struct
        {
            uint64_t data_load_misses;
            uint64_t data_load_miss_eligible_for_pseudo_hit_promotion;
            uint64_t data_load_miss_promoted_pseudo_hit;
        } pseudo_perfect;

    } stats;
    std::unordered_map<uint64_t, uint64_t> dependent_map;

    uint64_t total_miss_latency;

    /* Array of prefetchers associated with this cache */
    vector<Prefetcher*> prefetchers;
    vector<Prefetcher*> l1d_prefetchers;
    vector<Prefetcher*> llc_prefetchers;

    /* For semi-perfect cache */
    deque<uint64_t> page_buffer;

    /* For cache accuracy measurement */
    uint64_t cycle, next_measure_cycle;
    uint64_t pf_useful_epoch, pf_filled_epoch;
    uint32_t pref_acc;
    uint64_t total_acc_epochs, acc_epoch_hist[CACHE_ACC_LEVELS];

    CacheTracer tracer;

    CacheReplBase *llc_repl;

    unordered_map<uint64_t, std::pair<uint64_t, uint64_t> > crit_stats;

    // hitorgram of ROB position of loads missing in cache
    uint64_t missing_load_rob_pos_hist[ROB_SIZE];

    // To model probabilistic cache hit for performance headroom study
    std::default_random_engine generator;
    std::bernoulli_distribution *dist;

    // constructor
    // constructor
    CACHE(string v1, uint32_t v2, int v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8) 
        : NAME(v1), NUM_SET(v2), NUM_WAY(v3), NUM_LINE(v4), WQ_SIZE(v5), RQ_SIZE(v6), PQ_SIZE(v7), MSHR_SIZE(v8) 
    {
        LATENCY = 0;

        // cache block
        block = new BLOCK* [NUM_SET];
        for (uint32_t i=0; i<NUM_SET; i++) {
            block[i] = new BLOCK[NUM_WAY]; 
            for (uint32_t j=0; j<NUM_WAY; j++) {
                block[i][j].lru = j;
            }
        }

        for (uint32_t i=0; i<NUM_CPUS; i++) {
            upper_level_icache[i] = NULL;
            upper_level_dcache[i] = NULL;
        }

        lower_level = NULL;
        extra_interface = NULL;
        fill_level = -1;
        MAX_READ = 1;
        MAX_FILL = 1;

        llc_repl = NULL;
    }

    // destructor
    ~CACHE() {
        for (uint32_t i=0; i<NUM_SET; i++)
            delete[] block[i];
        delete[] block;
        delete RQ;
    };

    void reset_stats(uint32_t cpu)
    {
        for (uint32_t i=0; i<NUM_TYPES; i++) 
        {
            ACCESS[i] = 0;
            HIT[i] = 0;
            MISS[i] = 0;
            MSHR_MERGED[i] = 0;
            STALL[i] = 0;

            sim_access[cpu][i] = 0;
            sim_hit[cpu][i] = 0;
            sim_miss[cpu][i] = 0;
        }

        total_miss_latency = 0;

        pf_requested = 0;
        pf_issued = 0;
        pf_useful = 0;
        pf_useless = 0;
        pf_dropped = 0;
        pf_filled = 0;
        pf_late = 0;

        RQ->ACCESS = 0;
        RQ->MERGED = 0;
        RQ->TO_CACHE = 0;

        WQ.ACCESS = 0;
        WQ.MERGED = 0;
        WQ.TO_CACHE = 0;
        WQ.FORWARD = 0;
        WQ.FULL = 0;
        WQ.is_WQ = 1;

        bzero(&(stats), sizeof(stats));
        dependent_map.clear();
        for(uint32_t type = LOAD; type < NUM_TYPES; ++type) 
            stats.eviction.cat_reuse_min[type] = UINT64_MAX;
        stats.eviction.all_reuse_min = UINT64_MAX;

        stats.eviction.dep_all_min = UINT64_MAX;
        stats.eviction.dep_branch_mispred_min = UINT64_MAX;
        stats.eviction.dep_branch_min = UINT64_MAX;
        stats.eviction.dep_load_min = UINT64_MAX;

        for(uint32_t index = 0; index < ROB_SIZE; ++index)
            missing_load_rob_pos_hist[index] = 0;
    }

    void init_rand_engine(uint64_t seed, float prob)
    {
        generator.seed(seed);
        dist = new std::bernoulli_distribution(prob);
    }

    // functions
    void create_rq();

    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    int  check_hit(PACKET *packet),
         invalidate_entry(uint64_t inval_addr),
         check_mshr(PACKET *packet),
         prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, uint32_t prefetch_metadata),
         kpc_prefetch_line(uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, int delta, int depth, int signature, int confidence, uint32_t prefetch_metadata);

    void handle_fill(),
         handle_writeback(),
         handle_read(),
         handle_prefetch();

    void add_mshr(PACKET *packet),
         update_fill_cycle(),
         llc_initialize_replacement(),
         llc_replacement_print_config(),
         update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         lru_update(uint32_t set, uint32_t way),
         fill_cache(uint32_t set, uint32_t way, PACKET *packet),
         replacement_final_stats(),
         llc_replacement_final_stats(),
         //prefetcher_initialize(),
         l1d_prefetcher_initialize(),
         l2c_prefetcher_initialize(),
         llc_prefetcher_initialize(),
         prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
         l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
         prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr),
         l1d_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         //prefetcher_final_stats(),
         l1d_prefetcher_final_stats(),
         l2c_prefetcher_final_stats(),
         llc_prefetcher_final_stats();
    void (*l1i_prefetcher_cache_operate)(uint32_t, uint64_t, uint8_t, uint8_t);
    void (*l1i_prefetcher_cache_fill)(uint32_t, uint64_t, uint32_t, uint32_t, uint8_t, uint64_t);

    uint32_t l2c_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
         llc_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
         l2c_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         llc_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in);

    void l1d_prefetcher_print_config(),
        l2c_prefetcher_print_config(),
        llc_prefetcher_print_config();

    uint32_t l1d_prefetcher_prefetch_hit(uint64_t addr, uint64_t ip, uint32_t metadata_in),
            l2c_prefetcher_prefetch_hit(uint64_t addr, uint64_t ip, uint32_t metadata_in),
            llc_prefetcher_prefetch_hit(uint64_t addr, uint64_t ip, uint32_t metadata_in);
    
    uint32_t get_set(uint64_t address),
             get_way(uint64_t address, uint32_t set),
             find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             llc_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);

    void prefetcher_feedback(uint64_t &pref_gen, uint64_t &pref_fill, uint64_t &pref_used, uint64_t &pref_late);

    // @RBERA new functions
    void track_stats_from_victim(uint32_t set, uint32_t way);
    hit_where_t assign_hit_where(uint8_t cache_type, uint32_t where_in_cache);
    void send_signal_to_core(uint32_t cpu, PACKET packet);
        
    bool search_and_add(uint64_t page);
   
    void broadcast_bw(uint8_t bw_level),
        l1d_prefetcher_broadcast_bw(uint8_t bw_level),
        l2c_prefetcher_broadcast_bw(uint8_t bw_level),
        llc_prefetcher_broadcast_bw(uint8_t bw_level);

    void broadcast_ipc(uint8_t ipc),
        l1d_prefetcher_broadcast_ipc(uint8_t ipc),
        l2c_prefetcher_broadcast_ipc(uint8_t ipc),
        llc_prefetcher_broadcast_ipc(uint8_t ipc);
   
    void handle_prefetch_feedback();
   
    void broadcast_acc(uint32_t acc_level),
        l1d_prefetcher_broadcast_acc(uint32_t bw_level),
        l2c_prefetcher_broadcast_acc(uint32_t bw_level),
        llc_prefetcher_broadcast_acc(uint32_t bw_level);
};

void print_cache_config();

#endif
