/*********************************************************************
 * This file implements Tag-Tracking based Off-chip Predictor (TTP).
 * The core idea behind TTP is similar to MissMap [Loh+, MICRO'11]
 * and Location Map [Jalili+, HPCA'22].
 * 
 * TTP implements a catalog, which tries to track address tags of 
 * all cachelines present the cache hierarchy. For every LLC eviction
 * we evict the tag from the catalog.
 *
 * During prediction, TTP looks up the catalog with the load address.
 * If the tag is present in the catalog -> load will NOT go off-chip
 * Otherwise, the load will go off-chip.
 *
 * 
 * Author: Rahul Bera (write2bera@gmail.com)
 *********************************************************************/

#ifndef OFFCHIP_PRED_TTP_H
#define OFFCHIP_PRED_TTP_H

#include <unordered_map>
#include "offchip_pred_base.h"

class OffchipPredTTP : public OffchipPredBase
{
    private:
        /* Ideally, catalog_cache should only track partial tags of all cachelines.
         * Since LP also needs to track LLC evictions, we have added means to
         * track both virtual address tag and physical address tag of a cacheline.
         */
        vector<deque<pair<uint32_t, uint32_t>>> catalog_cache;

        /* This is a reverse index of catalog cache.
         * For a given physical address, it provides the set index 
         * of the corresponding virtual address in the catalog cache 
         */
        unordered_map<uint32_t, uint32_t> catalog_cache_rev_index;

        struct
        {
            struct
            {
                uint64_t called;
                uint64_t not_offchip;
                uint64_t not_offchip_and_in_catalog;
                uint64_t not_offchip_and_added_in_catalog;
                uint64_t went_offchip;
                uint64_t went_offchip_and_not_in_catalog;
                uint64_t went_offchip_and_deleted_from_catalog;
            } train;

            struct
            {
                uint64_t called;
                uint64_t physical_tag_not_found;
                uint64_t deleted_from_catalog_cache;
                uint64_t found_in_rev_index_not_found_in_catalog;
            } llc_eviction;
        } stats;

    private:
        void update_catalog_cache(uint64_t vaddr, uint64_t paddr, bool went_offchip);
        uint32_t get_partial_tag(uint64_t addr);
        bool lookup_catalog_cache(uint64_t addr);
        void add_to_catalog_cache_rev_index(uint32_t p_partial_tag, uint32_t set);
        void delete_from_catalog_cache_rev_index(uint32_t p_partial_tag);

    public:
        OffchipPredTTP(uint32_t _cpu, string _type, uint64_t _seed);
        ~OffchipPredTTP();

        void print_config();
        void dump_stats();
        void reset_stats();
        void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
        bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
        void track_llc_eviction(uint64_t paddr);
};

#endif /* OFFCHIP_PRED_TTP_H */


