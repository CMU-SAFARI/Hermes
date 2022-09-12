#include <iostream>
#include <algorithm>
#include "offchip_pred_ttp.h"
#include "util.h"

namespace knob
{
    extern uint32_t ocp_ttp_catalog_cache_sets;
    extern uint32_t ocp_ttp_catalog_cache_assoc;
    extern uint32_t ocp_ttp_hash_type;
    extern uint32_t ocp_ttp_partial_tag_size;
    extern bool     ocp_ttp_enable_track_llc_eviction;
}

void OffchipPredTTP::print_config()
{
    cout << "ocp_ttp_catalog_cache_sets " << knob::ocp_ttp_catalog_cache_sets << endl
         << "ocp_ttp_catalog_cache_assoc " << knob::ocp_ttp_catalog_cache_assoc << endl
         << "ocp_ttp_hash_type " << knob::ocp_ttp_hash_type << endl
         << "ocp_ttp_partial_tag_size " << knob::ocp_ttp_partial_tag_size << endl
         << "ocp_ttp_enable_track_llc_eviction " << knob::ocp_ttp_enable_track_llc_eviction << endl
         << endl;
}

void OffchipPredTTP::dump_stats()
{
    cout << "ocp_ttp_train_called " << stats.train.called << endl
         << "ocp_ttp_train_not_offchip " << stats.train.not_offchip << endl
         << "ocp_ttp_train_not_offchip_and_in_catalog " << stats.train.not_offchip_and_in_catalog << endl
         << "ocp_ttp_train_not_offchip_and_added_in_catalog " << stats.train.not_offchip_and_added_in_catalog << endl
         << "ocp_ttp_train_went_offchip " << stats.train.went_offchip << endl
         << "ocp_ttp_train_went_offchip_and_not_in_catalog " << stats.train.went_offchip_and_not_in_catalog << endl
         << "ocp_ttp_train_went_offchip_and_deleted_from_catalog " << stats.train.went_offchip_and_deleted_from_catalog << endl
         << endl
         << "ocp_ttp_llc_eviction_called " << stats.llc_eviction.called << endl
         << "ocp_ttp_llc_eviction_physical_tag_not_found " << stats.llc_eviction.physical_tag_not_found << endl
         << "ocp_ttp_llc_eviction_deleted_from_catalog_cache " << stats.llc_eviction.deleted_from_catalog_cache << endl
         << "ocp_ttp_llc_eviction_found_in_rev_index_not_found_in_catalog " << stats.llc_eviction.found_in_rev_index_not_found_in_catalog << endl
         << endl;
}

void OffchipPredTTP::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredTTP::OffchipPredTTP(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    // init catalog_cache
    deque<pair<uint32_t,uint32_t>> d;
    catalog_cache.resize(knob::ocp_ttp_catalog_cache_sets, d);
}

OffchipPredTTP::~OffchipPredTTP()
{

}

void OffchipPredTTP::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.train.called++;
    uint64_t vcladdr = lq_entry->virtual_address >> LOG2_BLOCK_SIZE;
    uint64_t pcladdr = lq_entry->physical_address >> LOG2_BLOCK_SIZE;

    update_catalog_cache(vcladdr, pcladdr, lq_entry->went_offchip);
}

bool OffchipPredTTP::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    uint64_t vcladdr = lq_entry->virtual_address >> LOG2_BLOCK_SIZE;

    bool found = lookup_catalog_cache(vcladdr);

    /* tag present in catalog cache means the access will likely hit in cache hierarchy.
     * Otherwise, it will likely go off-chip.
     */ 
    return found ? false : true;
}

void OffchipPredTTP::update_catalog_cache(uint64_t vaddr, uint64_t paddr, bool went_offchip)
{
    /* Catalog cache stores the partial tags of cachelines that did not go off-chip. 
     * We can conceptually think this as a 1-bit information per cacheline, as mentioned in the paper.
     */

    uint32_t v_partial_tag = get_partial_tag(vaddr);
    uint32_t p_partial_tag = get_partial_tag(paddr);

    uint32_t set = v_partial_tag % knob::ocp_ttp_catalog_cache_sets;
    auto it = find_if(catalog_cache[set].begin(), catalog_cache[set].end(), [v_partial_tag](pair<uint32_t, uint32_t> p){return p.first == v_partial_tag;});

    if(!went_offchip) // if hits in cache hierarchy
    {
        /* Either the partial tag is is already present,
         * or we need to insert it in catalog cache.
         */
        stats.train.not_offchip++;
        if(it != catalog_cache[set].end())
        {
            stats.train.not_offchip_and_in_catalog++;
            // do nothing
        }
        else
        {
            if(catalog_cache[set].size() >= knob::ocp_ttp_catalog_cache_assoc)
            {
                // evict the least-recently-used entry
                auto victim = catalog_cache[set].begin();

                // First, evict from reverse index cache
                delete_from_catalog_cache_rev_index(victim->second);

                // Second, evict from catalog cache
                catalog_cache[set].erase(victim);
            }

            // First, insert in catalog cache
            catalog_cache[set].push_back(pair<uint32_t, uint32_t>(v_partial_tag, p_partial_tag));

            // Second, insert in reverse index cache
            add_to_catalog_cache_rev_index(p_partial_tag, set);

            stats.train.not_offchip_and_added_in_catalog++;
        }
    }
    else // goes off-chip
    {
        /* Either the partial tag is not there in catalog cache,
         * or we need to remove it from there.
         */
        stats.train.went_offchip++;
        if(it == catalog_cache[set].end())
        {
            // do nothing
            stats.train.went_offchip_and_not_in_catalog++;
        }
        else
        {
            // First, evict from reverse index cache
            delete_from_catalog_cache_rev_index(p_partial_tag);

            // Second, evict from catalog cache
            catalog_cache[set].erase(it);
            
            stats.train.went_offchip_and_deleted_from_catalog++;
        }
    }
}

uint32_t OffchipPredTTP::get_partial_tag(uint64_t addr)
{
    uint32_t fxor = folded_xor(addr, 2);
    uint32_t hash = HashZoo::getHash(knob::ocp_ttp_hash_type, fxor);
    return hash & ((1u << knob::ocp_ttp_partial_tag_size) - 1);
}

bool OffchipPredTTP::lookup_catalog_cache(uint64_t addr)
{
    uint32_t partial_tag = get_partial_tag(addr);
    uint32_t set = partial_tag % knob::ocp_ttp_catalog_cache_sets;
    auto it = find_if(catalog_cache[set].begin(), catalog_cache[set].end(), [partial_tag](pair<uint32_t,uint32_t> p){return p.first == partial_tag;});
    return (it != catalog_cache[set].end()) ? true : false;
}

void OffchipPredTTP::add_to_catalog_cache_rev_index(uint32_t p_partial_tag, uint32_t set)
{
    /* search reverse index catalog with physical address tag */
    auto it = catalog_cache_rev_index.find(p_partial_tag);
    
    /* if the physical tag is not present, add it */
    if(it == catalog_cache_rev_index.end())
    {
        catalog_cache_rev_index.insert(pair<uint32_t, uint32_t>(p_partial_tag, set));
    }
    /* otherwise, update the location */
    else
    {
        it->second = set;
    } 
}

void OffchipPredTTP::delete_from_catalog_cache_rev_index(uint32_t p_partial_tag)
{
    /* search reverse index catalog with physical address tag */
    auto it = catalog_cache_rev_index.find(p_partial_tag);
    if(it != catalog_cache_rev_index.end())
    {
        catalog_cache_rev_index.erase(it);
    }
}

void OffchipPredTTP::track_llc_eviction(uint64_t paddr)
{
    if(!knob::ocp_ttp_enable_track_llc_eviction)
    {
        return;
    }

    uint64_t pcladdr = paddr >> LOG2_BLOCK_SIZE;
    uint32_t p_partial_tag = get_partial_tag(pcladdr);

    stats.llc_eviction.called++;
    
    /* search reverse index catalog with physical address tag */
    auto it = catalog_cache_rev_index.find(p_partial_tag);

    if(it == catalog_cache_rev_index.end())
    {
        stats.llc_eviction.physical_tag_not_found++;
        return;
    }

    uint32_t set = it->second;

    /* search for the matching way whose physical tag 
     * is the same as the tag of the evicted address
     */
    auto it2 = find_if(catalog_cache[set].begin(), catalog_cache[set].end(), [p_partial_tag](pair<uint32_t, uint32_t> p){return p.second == p_partial_tag;});
    
    /* if a matching entry is found in catalog cache, evict it */
    if(it2 != catalog_cache[set].end())
    {
        // First, evict from reverse index cache
        delete_from_catalog_cache_rev_index(p_partial_tag);

        // Second, evict from catalog cache
        catalog_cache[set].erase(it2);

        stats.llc_eviction.deleted_from_catalog_cache++;
    }
    else
    {
        // do nothing
        stats.llc_eviction.found_in_rev_index_not_found_in_catalog++;
    }
}