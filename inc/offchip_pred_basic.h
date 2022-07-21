#ifndef OFFCHIP_PRED_BASIC_H
#define OFFCHIP_PRED_BASIC_H

#include "util.h"
#include "offchip_pred_base.h"
#include "bitmap.h"
#include <unordered_set>

typedef enum
{
    ocp_basic_pc = 0,
    ocp_basic_pc_count,
    ocp_basic_pc_data_index,
    ocp_basic_pc_offset,
    ocp_basic_pc_page,
    ocp_basic_pc_addr,
    ocp_basic_cl_first_access,

    num_ocp_basic_feature_types
} ocp_basic_feature_type_t;

/* Feature set of OCP_Basic
 * Note: this feature set will be stored in LQ entry
 * and later used for training OCP */
class ocp_basic_feature_t : public ocp_base_feature_t
{
    public:
        uint32_t pc_count;
        bool     first_access;

        ocp_basic_feature_t()
        {
            pc_count = 0;
            first_access = false;
        }
        virtual ~ocp_basic_feature_t(){} // MUST be virtual
};

class ocp_basic_entry_t
{
    public:
        Counter total, miss;
        ocp_basic_entry_t()
        {

        }
        ~ocp_basic_entry_t() {}
};

class ocp_basic_pc_buf_entry_t
{
    public:
        uint64_t pc;
        uint32_t count;
    public:
        ocp_basic_pc_buf_entry_t()
        {
            pc = 0;
            count = 0;
        }
        ~ocp_basic_pc_buf_entry_t(){}
};

class ocp_basic_page_buf_entry_t
{
    public:
        uint64_t page;
        Bitmap bmp_access;
        uint32_t age;
    public:
        ocp_basic_page_buf_entry_t()
        {
            page = 0;
            bmp_access.reset();
            age = 0;
        }
};

class OffchipPredBasic : public OffchipPredBase
{
public:
    vector<ocp_basic_entry_t> m_table;
    deque<ocp_basic_pc_buf_entry_t> m_pc_buffer;
    deque<ocp_basic_page_buf_entry_t> m_page_buffer;
    unordered_set<uint64_t> unique_pcs;
    unordered_set<uint64_t> unique_pages;

    struct
    {
        struct
        {
            uint64_t called;
            uint64_t went_offchip;
        } train;

        struct
        {
            uint64_t called;
            uint64_t offchip_hit;
            uint64_t offchip_miss;
        } predict;

        struct
        {
            uint64_t called;
            uint64_t hit;
            uint64_t eviction;
            uint64_t insertion;
        } pc_buf;

        struct
        {
            uint64_t called;
            uint64_t hit;
            uint64_t eviction;
            uint64_t insertion;
        } page_buf;

    } stats;

    OffchipPredBasic(uint32_t _cpu, string _type, uint64_t _seed);
    ~OffchipPredBasic();

    uint32_t get_hash(uint64_t load_ip, uint32_t count, uint32_t data_index, uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool first_access);
    uint32_t get_hash_pc(uint64_t load_pc);
    uint32_t get_hash_pc_count(uint64_t load_pc, uint32_t count);
    uint32_t get_hash_pc_data_index(uint64_t load_pc, uint32_t data_index);
    uint32_t get_hash_pc_offset(uint64_t load_pc, uint32_t voffset);
    uint32_t get_hash_pc_page(uint64_t load_pc, uint64_t vpage);
    uint32_t get_hash_pc_addr(uint64_t load_pc, uint64_t vaddr);
    uint32_t get_hash_cl_first_access(bool first_access);

    void lookup_pc(uint64_t load_pc, uint32_t &count);
    void lookup_address(uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool &first_access);

    void print_config();
    void dump_stats();
    void reset_stats();
    void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
    bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
};

#endif /* OFFCHIP_PRED_BASIC_H */


