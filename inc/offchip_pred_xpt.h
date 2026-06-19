#ifndef OFFCHIP_PRED_XPT_H
#define OFFCHIP_PRED_XPT_H

#include "util.h"
#include "offchip_pred_base.h"
#include <deque>
#include <vector>

/* One entry of the XPT page tracker.
 * Tracks how many cachelines of a given physical page have gone off-chip. */
class ocp_xpt_tracker_entry_t
{
    public:
        uint64_t page;      // physical page number (tag)
        uint32_t offchip_count;

        ocp_xpt_tracker_entry_t()
        {
            page = 0;
            offchip_count = 0;
        }
        ~ocp_xpt_tracker_entry_t(){}
};

class OffchipPredXPT : public OffchipPredBase
{
public:
    // Tagged page tracker organized as a set-associative structure.
    // num_sets = ocp_xpt_num_entries / ocp_xpt_assoc
    // (e.g. 256 entries / 256-way => 1 set => fully associative).
    // A request to a page is predicted off-chip once the page's
    // off-chip cacheline count crosses ocp_xpt_offchip_threshold.
    uint32_t num_sets;

    // per set: front = LRU, back = MRU
    vector<deque<ocp_xpt_tracker_entry_t>> m_tracker;

    struct
    {
        struct
        {
            uint64_t called;
            uint64_t offchip;
            uint64_t not_offchip;
            uint64_t tracker_hit;
            uint64_t tracker_miss;
        } predict;

        struct
        {
            uint64_t called;
            uint64_t went_offchip;
        } train;

        struct
        {
            uint64_t hit;
            uint64_t eviction;
            uint64_t insertion;
        } tracker;

    } stats;

    OffchipPredXPT(uint32_t _cpu, string _type, uint64_t _seed);
    ~OffchipPredXPT();

    uint32_t get_set(uint64_t page);

    // shared core/uncore logic, parameterized by the already-selected address
    bool predict_helper(uint64_t addr);
    void train_helper(uint64_t addr, bool went_offchip);

    void print_config();
    void dump_stats();
    void reset_stats();
    void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
    bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);

    // Uncore (beside-LLC) path: operate on the PACKET (physical address available).
    void train(PACKET *packet);
    bool predict(PACKET *packet);
};

#endif /* OFFCHIP_PRED_XPT_H */
