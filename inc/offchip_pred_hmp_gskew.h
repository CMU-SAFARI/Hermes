/*****************************************************************************************
 * This file implements the the gskew version of Hit-miss predictor (HMP) introduced in:
 * Yoaz, A., Erez, M., Ronen, R., & Jourdan, S., 
 * "Speculation techniques for improving load related instruction scheduling", ISCA 1999
 * https://doi.org/10.1145/307338.300983
 *****************************************************************************************
 */

#ifndef OFFCHIP_PRED_HMP_GSKEW_H
#define OFFCHIP_PRED_HMP_GSKEW_H

#include "offchip_pred_base.h"
#include "hmp_commons.h"

#define MAX_HASHES 15

class OffchipPredHMPGskew : public OffchipPredBase
{
    private:
        uint32_t GHR;
        vector<vector<confidence_state_t>> PHT;
        
        /* stats */
        struct
        {
            struct
            {
                uint64_t called;
            } predict;

            struct
            {
                uint64_t called;
                uint64_t strong_nt_to_weak_nt[MAX_HASHES];
                uint64_t strong_nt_to_strong_nt[MAX_HASHES];
                uint64_t weak_nt_to_weak_t[MAX_HASHES];
                uint64_t weak_nt_to_strong_nt[MAX_HASHES];
                uint64_t weak_t_to_strong_t[MAX_HASHES];
                uint64_t weak_t_to_weak_nt[MAX_HASHES];
                uint64_t strong_t_to_strong_t[MAX_HASHES];
                uint64_t strong_t_to_weak_t[MAX_HASHES];
            } train;
        } stats;
        
    private:
        vector<uint32_t> get_hashes(uint64_t pc);
        confidence_state_t update_confidence_counter(uint32_t hash_id, confidence_state_t old_conf, bool went_offchip);

    public:
        OffchipPredHMPGskew(uint32_t cpu, string _type, uint64_t _seed);
        ~OffchipPredHMPGskew();
        
        /* interface functions */
        void print_config();
        void dump_stats();
        void reset_stats();
        void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
        bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
};

#endif /* OFFCHIP_PRED_HMP_GSKEW_H */

