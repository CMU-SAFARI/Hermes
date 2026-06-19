#include <iostream>
#include <algorithm>
#include "string.h"
#include "util.h"
#include "offchip_pred_xpt.h"
#include "ooo_cpu.h"

#if 0
    #define MYLOG(cond, ...)                                        \
        if (cond)                                                   \
        {                                                           \
            fprintf(stdout, "[%25s@%3u] ", __FUNCTION__, __LINE__); \
            fprintf(stdout, __VA_ARGS__);                           \
            fprintf(stdout, "\n");                                  \
            fflush(stdout);                                         \
        }
#else
#	define MYLOG(cond, ...) {}
#endif

namespace knob
{
    extern uint32_t ocp_xpt_num_entries;
    extern uint32_t ocp_xpt_assoc;
    extern uint32_t ocp_xpt_offchip_threshold;
    extern uint32_t ocp_xpt_hash_type;
    extern bool     ocp_xpt_use_physical_address;
    extern string   offchip_pred_location;
}

//=============================================================================
// Common: config, stats, construction, and shared helpers (both placements)
//=============================================================================

void OffchipPredXPT::print_config()
{
    cout << "ocp_xpt_num_entries " << knob::ocp_xpt_num_entries << endl
         << "ocp_xpt_assoc " << knob::ocp_xpt_assoc << endl
         << "ocp_xpt_offchip_threshold " << knob::ocp_xpt_offchip_threshold << endl
         << "ocp_xpt_hash_type " << knob::ocp_xpt_hash_type << endl
         << "ocp_xpt_use_physical_address " << knob::ocp_xpt_use_physical_address << endl
         << "ocp_xpt_num_sets " << num_sets << endl
         << endl;
}

void OffchipPredXPT::dump_stats()
{
    cout << "ocp_xpt_predict_called " << stats.predict.called << endl
         << "ocp_xpt_predict_offchip " << stats.predict.offchip << endl
         << "ocp_xpt_predict_not_offchip " << stats.predict.not_offchip << endl
         << "ocp_xpt_predict_tracker_hit " << stats.predict.tracker_hit << endl
         << "ocp_xpt_predict_tracker_miss " << stats.predict.tracker_miss << endl
         << "ocp_xpt_train_called " << stats.train.called << endl
         << "ocp_xpt_train_went_offchip " << stats.train.went_offchip << endl
         << "ocp_xpt_tracker_hit " << stats.tracker.hit << endl
         << "ocp_xpt_tracker_eviction " << stats.tracker.eviction << endl
         << "ocp_xpt_tracker_insertion " << stats.tracker.insertion << endl
         << endl;
}

void OffchipPredXPT::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredXPT::OffchipPredXPT(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    bzero(&stats, sizeof(stats));

    // XPT is natively a physical-page-indexed, LLC-side predictor. At the CORE, predict()
    // runs at add_load_queue() before translation, where the physical address is still 0 —
    // so physical indexing is meaningless and we forbid it there. At the LLC (uncore) the
    // physical address IS available, so physical indexing is allowed; only bar it core-side.
    if (knob::ocp_xpt_use_physical_address && !knob::offchip_pred_location.compare("core"))
    {
        cerr << "[XPT] ERROR: ocp_xpt_use_physical_address=true, but XPT is instantiated inside the core." << endl
             << "[XPT] The physical address is not available at prediction time (add_load_queue), so XPT" << endl
             << "[XPT] must index by virtual address when placed inside the core. Set" << endl
             << "[XPT] ocp_xpt_use_physical_address=false, or move XPT beside the LLC to use physical addresses." << endl;
        assert(false && "XPT core-side cannot use physical address");
    }

    assert(knob::ocp_xpt_assoc > 0 && knob::ocp_xpt_num_entries >= knob::ocp_xpt_assoc);
    // num_entries must be an exact multiple of assoc, otherwise integer division
    // silently drops the remainder entries (e.g. 257/256 => 1 set of 256).
    assert(knob::ocp_xpt_num_entries % knob::ocp_xpt_assoc == 0);
    num_sets = knob::ocp_xpt_num_entries / knob::ocp_xpt_assoc;

    for (uint32_t index = 0; index < num_sets; ++index)
    {
        deque<ocp_xpt_tracker_entry_t> d;
        d.clear();
        m_tracker.push_back(d);
    }
}

OffchipPredXPT::~OffchipPredXPT()
{

}

uint32_t OffchipPredXPT::get_set(uint64_t page)
{
    uint32_t folded_page = folded_xor(page, 2);
    uint32_t hash = HashZoo::getHash(knob::ocp_xpt_hash_type, folded_page);
    return hash % num_sets;
}

// Shared prediction logic. `addr` has already been selected by the caller (virtual
// or physical, core- or uncore-side); everything below is placement-agnostic.
bool OffchipPredXPT::predict_helper(uint64_t addr)
{
    uint64_t page = addr >> LOG2_PAGE_SIZE;
    uint32_t set = get_set(page);

    stats.predict.called++;

    // look up the page in the tracker set
    auto it = find_if(m_tracker[set].begin(), m_tracker[set].end(),
                      [page](const ocp_xpt_tracker_entry_t &entry) { return entry.page == page; });

    bool prediction = false;
    if (it != m_tracker[set].end()) // page is being tracked
    {
        stats.predict.tracker_hit++;
        // predict off-chip once enough cachelines of this page have missed the LLC
        prediction = (it->offchip_count >= knob::ocp_xpt_offchip_threshold);
    }
    else // page is not being tracked
    {
        stats.predict.tracker_miss++;
    }

    if (prediction) stats.predict.offchip++;
    else            stats.predict.not_offchip++;

    return prediction;
}

// Shared training logic. `addr` already selected by the caller; `went_offchip` is the
// resolved outcome (LSQ_ENTRY::went_offchip core-side, PACKET::went_offchip uncore-side).
void OffchipPredXPT::train_helper(uint64_t addr, bool went_offchip)
{
    uint64_t page = addr >> LOG2_PAGE_SIZE;
    uint32_t set = get_set(page);

    stats.train.called++;
    if (went_offchip) stats.train.went_offchip++;

    // look up the page in the tracker set
    auto it = find_if(m_tracker[set].begin(), m_tracker[set].end(),
                      [page](const ocp_xpt_tracker_entry_t &entry) { return entry.page == page; });

    if (it != m_tracker[set].end()) // tracker hit
    {
        stats.tracker.hit++;
        ocp_xpt_tracker_entry_t entry = (*it);
        if (went_offchip) entry.offchip_count++;
        // move to MRU position
        m_tracker[set].erase(it);
        m_tracker[set].push_back(entry);
    }
    else // tracker miss -> allocate a new entry
    {
        if (m_tracker[set].size() >= knob::ocp_xpt_assoc) // evict LRU
        {
            m_tracker[set].pop_front();
            stats.tracker.eviction++;
        }

        ocp_xpt_tracker_entry_t entry;
        entry.page = page;
        entry.offchip_count = went_offchip ? 1 : 0;
        m_tracker[set].push_back(entry);
        stats.tracker.insertion++;
    }
}

//=============================================================================
// Core-side placement (offchip_pred_location == core): LSQ_ENTRY interface
//=============================================================================

bool OffchipPredXPT::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    // XPT is natively physical-page-indexed; core-side it falls back to virtual page
    // (see constructor). The knob selects which address to use.
    uint64_t addr = knob::ocp_xpt_use_physical_address ? lq_entry->physical_address : lq_entry->virtual_address;
    return predict_helper(addr);
}

void OffchipPredXPT::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    uint64_t addr = knob::ocp_xpt_use_physical_address ? lq_entry->physical_address : lq_entry->virtual_address;
    train_helper(addr, lq_entry->went_offchip);
}

//=============================================================================
// Uncore-side placement (LLC; offchip_pred_location == uncore): PACKET interface
//=============================================================================

bool OffchipPredXPT::predict(PACKET *packet)
{
    // beside the LLC the physical address is available on the PACKET. (PACKET has no
    // virtual address yet — to be added later to compare virtual vs physical indexing.)
    return predict_helper(packet->full_addr);
}

void OffchipPredXPT::train(PACKET *packet)
{
    train_helper(packet->full_addr, packet->went_offchip);
}
