/********************************************************
 * This file implements the level predictor proposed in 
 * "Reducing Load Latency with Cache Level Prediction", HPCA 2022
 * Note that, this is a hardware-constrained implementation,
 * meaning, unlike the original LP, this version does not
 * store the metadata in the main memory space.
 *
 * Author: Rahul Bera (write2bera@gmail.com)
 ********************************************************/

#ifndef OFFCHIP_PRED_LP_H
#define OFFCHIP_PRED_LP_H

#include "offchip_pred_base.h"

class OffchipPredLP : public OffchipPredBase
{
    private:
        vector<deque<uint32_t>> catalog_cache;

        struct
        {
            struct
            {
                uint64_t called;
                uint64_t went_offchip;
                uint64_t went_offchip_and_in_catalog;
                uint64_t went_offchip_and_added_in_catalog;
                uint64_t not_offchip;
                uint64_t not_offchip_and_not_in_catalog;
                uint64_t not_offchip_and_deleted_from_catalog;
            } train;
        } stats;

    private:
        void update_catalog_cache(uint64_t addr, bool went_offchip);
        uint32_t get_partial_tag(uint64_t addr);
        bool lookup_catalog_cache(uint64_t addr);

    public:
        OffchipPredLP(uint32_t _cpu, string _type, uint64_t _seed);
        ~OffchipPredLP();

        void print_config();
        void dump_stats();
        void reset_stats();
        void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
        bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
};

#endif /* OFFCHIP_PRED_LP_H */


