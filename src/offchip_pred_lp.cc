#include <iostream>
#include <algorithm>
#include "offchip_pred_lp.h"
#include "util.h"

namespace knob
{
    extern uint32_t ocp_lp_catalog_cache_sets;
    extern uint32_t ocp_lp_catalog_cache_assoc;
    extern uint32_t ocp_lp_hash_type;
    extern uint32_t ocp_lp_partial_tag_size;
}

void OffchipPredLP::print_config()
{
    cout << "ocp_lp_catalog_cache_sets " << knob::ocp_lp_catalog_cache_sets << endl
         << "ocp_lp_catalog_cache_assoc " << knob::ocp_lp_catalog_cache_assoc << endl
         << "ocp_lp_hash_type " << knob::ocp_lp_hash_type << endl
         << "ocp_lp_partial_tag_size " << knob::ocp_lp_partial_tag_size << endl
         << endl;
}

void OffchipPredLP::dump_stats()
{
    cout << "ocp_lp_train_called " << stats.train.called << endl
         << "ocp_lp_train_went_offchip " << stats.train.went_offchip << endl
         << "ocp_lp_train_went_offchip_and_in_catalog " << stats.train.went_offchip_and_in_catalog << endl
         << "ocp_lp_train_went_offchip_and_added_in_catalog " << stats.train.went_offchip_and_added_in_catalog << endl
         << "ocp_lp_train_not_offchip " << stats.train.not_offchip << endl
         << "ocp_lp_train_not_offchip_and_not_in_catalog " << stats.train.not_offchip_and_not_in_catalog << endl
         << "ocp_lp_train_not_offchip_and_deleted_from_catalog " << stats.train.not_offchip_and_deleted_from_catalog << endl
         << endl;
}

void OffchipPredLP::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredLP::OffchipPredLP(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    // init catalog_cache
    deque<uint32_t> d;
    catalog_cache.resize(knob::ocp_lp_catalog_cache_sets, d);
}

OffchipPredLP::~OffchipPredLP()
{

}

void OffchipPredLP::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.train.called++;
    uint64_t vcladdr = lq_entry->virtual_address >> LOG2_BLOCK_SIZE;

    update_catalog_cache(vcladdr, lq_entry->went_offchip);
}

bool OffchipPredLP::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    uint64_t vcladdr = lq_entry->virtual_address >> LOG2_BLOCK_SIZE;

    bool found = lookup_catalog_cache(vcladdr);
    return found ? true : false;
}

void OffchipPredLP::update_catalog_cache(uint64_t addr, bool went_offchip)
{
    /* Catalog cache stores the partial tags of cachelines that went to off-chip. 
     * We can conceptually think this as a 1-bit information per cacheline, as mentioned in the paper.
     */

    uint32_t partial_tag = get_partial_tag(addr);
    uint32_t set = partial_tag % knob::ocp_lp_catalog_cache_sets;
    auto it = find_if(catalog_cache[set].begin(), catalog_cache[set].end(), [partial_tag](uint32_t val){return val == partial_tag;});

    if(went_offchip)
    {
        /* Either the partial tag is is already present,
         * or we need to insert it in catalog cache.
         */
        stats.train.went_offchip++;
        if(it != catalog_cache[set].end())
        {
            stats.train.went_offchip_and_in_catalog++;
            // do nothing
        }
        else
        {
            if(catalog_cache[set].size() >= knob::ocp_lp_catalog_cache_assoc)
            {
                // evict the least-recently-used entry
                catalog_cache[set].pop_front();
            }
            catalog_cache[set].push_back(partial_tag);
            stats.train.went_offchip_and_added_in_catalog++;
        }
    }
    else
    {
        /* Either the partial tag is not there in catalog cache,
         * or we need to remove it from there.
         */
        stats.train.not_offchip++;
        if(it == catalog_cache[set].end())
        {
            // do nothing
            stats.train.not_offchip_and_not_in_catalog++;
        }
        else
        {
            // delete from catalog cache
            catalog_cache[set].erase(it);
            stats.train.not_offchip_and_deleted_from_catalog++;
        }
    }
}

uint32_t OffchipPredLP::get_partial_tag(uint64_t addr)
{
    uint32_t fxor = folded_xor(addr, 2);
    uint32_t hash = HashZoo::getHash(knob::ocp_lp_hash_type, fxor);
    return hash & ((1u << knob::ocp_lp_partial_tag_size) - 1);
}

bool OffchipPredLP::lookup_catalog_cache(uint64_t addr)
{
    uint32_t partial_tag = get_partial_tag(addr);
    uint32_t set = partial_tag % knob::ocp_lp_catalog_cache_sets;
    auto it = find_if(catalog_cache[set].begin(), catalog_cache[set].end(), [partial_tag](uint32_t val){return val == partial_tag;});
    return (it != catalog_cache[set].end()) ? true : false;
}