#ifndef BLOCK_H
#define BLOCK_H

#include <cstring>
#include <deque>
#include "champsim.h"
#include "instruction.h"
#include "set.h"
#include "cache_tracer.h"
#include "defs.h"
#include "offchip_pred_base_helper.h"

// CACHE BLOCK
class BLOCK {
  public:
    uint8_t valid,
            prefetch,
            dirty,
            used;

    int delta,
        depth,
        signature,
        confidence;

    uint64_t address,
             full_addr,
             tag,
             data,
             ip,
             cpu,
             instr_id;

    // replacement state
    uint32_t lru;

    // RBERA: metadata
    uint32_t reuse[NUM_TYPES];
    uint32_t dependents;
    uint32_t cat_dependents[DEP_INSTR_TYPES];
    bool critical_actual;
    bool critical_pred;
    uint64_t fill_ip;
    uint32_t reuse_frontal_dorsal[NUM_PARTITION_TYPES];

    BLOCK() {
        valid = 0;
        prefetch = 0;
        dirty = 0;
        used = 0;

        delta = 0;
        depth = 0;
        signature = 0;
        confidence = 0;

        address = 0;
        full_addr = 0;
        tag = 0;
        data = 0;
        cpu = 0;
        instr_id = 0;

        lru = 0;

        reset_metadata();
    };
    
    void reset_metadata()
    {
        for(uint32_t index = 0; index < NUM_TYPES; ++index) 
        {
            reuse[index] = 0;
        }
        for(uint32_t index = 0; index < NUM_PARTITION_TYPES; ++index)
        {
            reuse_frontal_dorsal[index] = 0;
        }
        dependents = 0;
        for(uint32_t index = 0; index < DEP_INSTR_TYPES; ++index) 
        {
            cat_dependents[index] = 0;
        }
        critical_actual = false;
        critical_pred = false;
        fill_ip = 0xdeadbeef;
    }
};

// DRAM CACHE BLOCK
class DRAM_ARRAY {
  public:
    BLOCK **block;

    DRAM_ARRAY() {
        block = NULL;
    };
};

typedef enum
{
    INV = 0,
    ITLB,
    ITLB_MSHR,
    DTLB,
    DTLB_MSHR,
    STLB, // STLB does not have MSHR. Anything that misses STLB directly emiulates PTW
    PTW,
    L1I,
    L1I_RQ,
    L1I_WQ,
    L1I_MSHR,
    L1D,
    L1D_RQ,
    L1D_WQ,
    L1D_MSHR,
    L2C,
    L2C_RQ,
    L2C_WQ,
    L2C_MSHR,
    LLC,
    LLC_RQ,
    LLC_WQ,
    LLC_MSHR,
    DRAM,

    NumHitWheres
} hit_where_t;

// message packet
class PACKET {
  public:
    uint64_t id; // just a packet id
    static uint64_t next_id;

    uint8_t instruction, 
            is_data,
            fill_l1i,
            fill_l1d,
            tlb_access,
            scheduled,
            translated,
            fetched,
            prefetched,
            drc_tag_read;

    int fill_level, 
        pf_origin_level,
        rob_signal, 
        rob_index,
        rob_position,
        producer,
        delta,
        depth,
        signature,
        confidence;

    int8_t rob_part_type;

    hit_where_t hit_where;

    uint32_t pf_metadata;

    uint8_t  is_producer, 
             //rob_index_depend_on_me[ROB_SIZE], 
             //lq_index_depend_on_me[ROB_SIZE], 
             //sq_index_depend_on_me[ROB_SIZE], 
             instr_merged,
             load_merged, 
             store_merged,
             returned,
             asid[2],
             type;

    fastset
             rob_index_depend_on_me, 
             lq_index_depend_on_me, 
             sq_index_depend_on_me;

    uint32_t cpu, data_index, lq_index, sq_index;

    uint64_t address, 
             full_addr, 
             instruction_pa,
             data_pa,
             data,
             instr_id,
             ip, 
             event_cycle,
             cycle_enqueued,
             enque_cycle[NUM_MODULE_TYPES][NUM_QUEUE_TYPES],
             deque_cycle[NUM_MODULE_TYPES][NUM_QUEUE_TYPES];

    uint8_t went_offchip_pred; // populated from corresponding LQ entry

    PACKET() {
        id = next_id++;

        instruction = 0;
        is_data = 1;
	    fill_l1i = 0;
	    fill_l1d = 0;
        tlb_access = 0;
        scheduled = 0;
        translated = 0;
        fetched = 0;
        prefetched = 0;
        drc_tag_read = 0;

        returned = 0;
        asid[0] = UINT8_MAX;
        asid[1] = UINT8_MAX;
        type = 0;

        fill_level = -1; 
        rob_signal = -1;
        rob_index = -1;
        rob_position = -1;
        producer = -1;
        delta = 0;
        depth = 0;
        signature = 0;
        confidence = 0;

        rob_part_type = -1;

        hit_where = hit_where_t::INV;

#if 0
        for (uint32_t i=0; i<ROB_SIZE; i++) {
            rob_index_depend_on_me[i] = 0;
            lq_index_depend_on_me[i] = 0;
            sq_index_depend_on_me[i] = 0;
        }
#endif
        is_producer = 0;
        instr_merged = 0;
        load_merged = 0;
        store_merged = 0;

        cpu = NUM_CPUS;
        data_index = 0;
        lq_index = 0;
        sq_index = 0;

        address = 0;
        full_addr = 0;
        instruction_pa = 0;
        data = 0;
        instr_id = 0;
        ip = 0;
        event_cycle = UINT64_MAX;
	    cycle_enqueued = 0;
        for(uint32_t i = 0; i < NUM_MODULE_TYPES; ++i)
        {
            for(uint32_t j = 0; j < NUM_QUEUE_TYPES; ++j)
            {
                enque_cycle[i][j] = 0;
                deque_cycle[i][j] = 0;
            }
        }
        went_offchip_pred = 0;
    };
    std::string to_string();
};

class PACKET_QUEUE_BASE
{
  public:
    string NAME;
    uint32_t SIZE;

    uint8_t  is_RQ, 
             is_WQ,
             write_mode;

    uint32_t cpu, 
             occupancy, 
             num_returned, 
             next_fill_index, 
             next_schedule_index, 
             next_process_index;

    uint64_t next_fill_cycle, 
             next_schedule_cycle, 
             next_process_cycle,
             ACCESS,
             FORWARD,
             MERGED,
             TO_CACHE,
             ROW_BUFFER_HIT,
             ROW_BUFFER_MISS,
             FULL;

    int8_t module_type, queue_type;

    // some stats
    struct
    {
        uint64_t requests_seen[NUM_PARTITION_TYPES];
        uint64_t total_queueing_delay[NUM_PARTITION_TYPES];
    } stats;

    PACKET_QUEUE_BASE(string v1, uint32_t v2) : NAME(v1), SIZE(v2)
    {
        is_RQ = 0;
        is_WQ = 0;
        write_mode = 0;

        cpu = 0; 
        occupancy = 0;
        num_returned = 0;
        next_fill_index = 0;
        next_schedule_index = 0;
        next_process_index = 0;

        next_fill_cycle = UINT64_MAX;
        next_schedule_cycle = UINT64_MAX;
        next_process_cycle = UINT64_MAX;

        ACCESS = 0;
        FORWARD = 0;
        MERGED = 0;
        TO_CACHE = 0;
        ROW_BUFFER_HIT = 0;
        ROW_BUFFER_MISS = 0;
        FULL = 0;

        module_type = get_module_type();
        queue_type = get_queue_type();
        reset_stats();
    }

    PACKET_QUEUE_BASE()
    {
        is_RQ = 0;
        is_WQ = 0;

        cpu = 0; 
        occupancy = 0;
        num_returned = 0;
        next_fill_index = 0;
        next_schedule_index = 0;
        next_process_index = 0;

        next_fill_cycle = UINT64_MAX;
        next_schedule_cycle = UINT64_MAX;
        next_process_cycle = UINT64_MAX;

        ACCESS = 0;
        FORWARD = 0;
        MERGED = 0;
        TO_CACHE = 0;
        ROW_BUFFER_HIT = 0;
        ROW_BUFFER_MISS = 0;
        FULL = 0;

        module_type = -1;
        queue_type = -1;
        reset_stats();
    }

    virtual void init(uint32_t size) = 0;

    virtual ~PACKET_QUEUE_BASE() {} // has to be virtual

    void reset_stats()
    {
        bzero(&stats, sizeof(stats));
    }

    int8_t get_module_type();
    int8_t get_queue_type();
    void deduce_module_queue_types();

    // to collect and print queueing stats
    void record_queue_stats(PACKET *packet);
    void dump_stats();

    // interface functions
    virtual int check_queue(PACKET* packet, uint64_t timestamp = 0) = 0;
    virtual int add_queue(PACKET* packet, uint64_t timestamp = 0) = 0;
    virtual void remove_queue(PACKET* packet, uint64_t timestamp = 0) = 0;
    virtual bool is_empty() = 0;
    virtual PACKET& get_entry(int32_t index) = 0;
    virtual int32_t get_head() = 0;
    virtual int32_t get_tail() = 0;
    virtual PACKET& peek() = 0;
};

// basic FIFO packet queue
class PACKET_QUEUE : public PACKET_QUEUE_BASE
{
public:
    uint32_t head, tail;
    PACKET *entry;

    // constructor
    PACKET_QUEUE(string v1, uint32_t v2) : PACKET_QUEUE_BASE(v1, v2)
    {
        head = 0;
        tail = 0;
        entry = new PACKET[SIZE];
    };

    PACKET_QUEUE() : PACKET_QUEUE_BASE()
    {
        head = 0;
        tail = 0;
        //entry = new PACKET[SIZE]; 
    };

    void init(uint32_t size)
    {
        SIZE = size;
        head = 0;
        tail = 0;
        entry = new PACKET[SIZE]; 
    }

    // destructor
    virtual ~PACKET_QUEUE() 
    {
        delete[] entry;
    };

    // override functions
    int check_queue(PACKET* packet, uint64_t timestamp = 0);
    int add_queue(PACKET* packet, uint64_t timestamp = 0);
    void remove_queue(PACKET* packet, uint64_t timestamp = 0);
    inline bool is_empty() {return entry[head].cpu == NUM_CPUS ? true : false;}
    inline PACKET& get_entry(int32_t index) {return entry[index];}
    inline int32_t get_head() {return head;}
    inline int32_t get_tail() {return tail;}
    inline PACKET& peek() {return get_entry(get_head());}
};

typedef enum
{
    FCFS = 0,               // 0
    ROB_POS_FCFS,           // 1
    OFFCHIP_FCFS,           // 2
    OFFCHIP_ROB_POS_FCFS,   // 3

    NumPriorityTypes
} priority_type_t;

extern string priority_name_string[NumPriorityTypes];

// priority-based packet queue
class PACKET_QUEUE_PRIORITY : public PACKET_QUEUE_BASE
{
public:
    // head always points to the entry with the highest priority
    // -1 if occupancy is zero
    int32_t head;
    deque<PACKET>::iterator head_iterator; // iterator to head; used in remove_queue()
    priority_type_t priority_type;

    deque<PACKET> entry;

    // constructors
    PACKET_QUEUE_PRIORITY(string v1, uint32_t v2, priority_type_t v3) : PACKET_QUEUE_BASE(v1, v2), priority_type(v3)
    {
        entry.clear();
        occupancy = 0;
        head = -1;
    }

    PACKET_QUEUE_PRIORITY(priority_type_t v1) : PACKET_QUEUE_BASE(), priority_type(v1)
    {
        head = -1;
        occupancy = 0;
    }

    void init(uint32_t size)
    {
        SIZE = size;
        entry.clear();
        occupancy = 0;
        head = -1;
    }

    // destructor
    virtual ~PACKET_QUEUE_PRIORITY()
    {

    };

    // override functions
    int check_queue(PACKET* packet, uint64_t timestamp = 0);
    int add_queue(PACKET* packet, uint64_t timestamp = 0);
    void remove_queue(PACKET* packet, uint64_t timestamp = 0);
    inline bool is_empty() {return head == -1 ? true : false;}
    inline PACKET& get_entry(int32_t index) {return entry[index];}
    inline int32_t get_head() {return head;}
    inline int32_t get_tail() {return -1;}
    inline PACKET& peek() {return get_entry(get_head());}
    
private:
    // returns +1 if packet1 has higher priority than packet2
    //         -1 if packet1 has lower  priority than packet2
    //         0 if their priority is same
    int32_t compare_priorty(PACKET packet1, PACKET packet2);
    int32_t compare_priorty(deque<PACKET>::iterator it1, deque<PACKET>::iterator it2);

    // different priority comparison functions
    int32_t priority_FCFS(PACKET packet1, PACKET packet2);
    int32_t priority_ROB_POS_FCFS(PACKET packet1, PACKET packet2);
    int32_t priority_OFFCHIP_FCFS(PACKET packet1, PACKET packet2);
    int32_t priority_OFFCHIP_ROB_POS_FCFS(PACKET packet1, PACKET packet2);
};

// reorder buffer
class CORE_BUFFER {
  public:
    const string NAME;
    const uint32_t SIZE;
    uint32_t cpu, 
             head, 
             tail,
             occupancy,
             next_schedule;
    uint64_t event_cycle,
             fetch_event_cycle,
             schedule_event_cycle,
             execute_event_cycle,
             lsq_event_cycle,
             retire_event_cycle;

    ooo_model_instr *entry;

    // constructor
    CORE_BUFFER(string v1, uint32_t v2) : NAME(v1), SIZE(v2) {
        head = 0;
        tail = 0;
        occupancy = 0;

        next_schedule = 0;

        event_cycle = 0;
        fetch_event_cycle = UINT64_MAX;
        schedule_event_cycle = UINT64_MAX;
        execute_event_cycle = UINT64_MAX;
        lsq_event_cycle = UINT64_MAX;
        retire_event_cycle = UINT64_MAX;

        entry = new ooo_model_instr[SIZE];
    };

    // destructor
    ~CORE_BUFFER() {
        delete[] entry;
    };
};

// load/store queue 
class LSQ_ENTRY {
  public:
    uint64_t instr_id,
             producer_id,
             virtual_address,
             physical_address,
             ip,
             event_cycle;

    uint32_t rob_index, 
             data_index, 
             sq_index;

    int rob_position;
    uint8_t rob_part_type;

    uint8_t translated,
            fetched,
            asid[2];
    
    // forwarding_depend_on_me[ROB_SIZE];
    fastset forwarding_depend_on_me;
    
    uint8_t went_offchip; // 1 => this request went to DRAM
    uint8_t went_offchip_pred; // predicted to go off-chip
    ocp_base_feature_t *ocp_feature;

    // constructor
    LSQ_ENTRY() {
        instr_id = 0;
        producer_id = UINT64_MAX;
        virtual_address = 0;
        physical_address = 0;
        ip = 0;
        event_cycle = 0;

        rob_index = 0;
        data_index = 0;
        sq_index = UINT32_MAX;
        
        rob_position = -1;
        rob_part_type = -1;

        translated = 0;
        fetched = 0;
        asid[0] = UINT8_MAX;
        asid[1] = UINT8_MAX;

        went_offchip = 0;
        went_offchip_pred = 0;
        ocp_feature = NULL;

#if 0
        for (uint32_t i=0; i<ROB_SIZE; i++)
            forwarding_depend_on_me[i] = 0;
#endif
    };
};

class LOAD_STORE_QUEUE {
  public:
    const string NAME;
    const uint32_t SIZE;
    uint32_t occupancy, head, tail;

    LSQ_ENTRY *entry;

    // constructor
    LOAD_STORE_QUEUE(string v1, uint32_t v2) : NAME(v1), SIZE(v2) {
        occupancy = 0;
        head = 0;
        tail = 0;

        entry = new LSQ_ENTRY[SIZE];
    };

    // destructor
    ~LOAD_STORE_QUEUE() {
        delete[] entry;
    };
};
#endif
