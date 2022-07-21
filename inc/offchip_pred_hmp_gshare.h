/*****************************************************************************************
 * This file implements the the gshare version of Hit-miss predictor (HMP) introduced in:
 * Yoaz, A., Erez, M., Ronen, R., & Jourdan, S., 
 * "Speculation techniques for improving load related instruction scheduling", ISCA 1999
 * https://doi.org/10.1145/307338.300983
 *****************************************************************************************
 */

#ifndef OFFCHIP_PRED_HMP_GSHARE_H
#define OFFCHIP_PRED_HMP_GSHARE_H

#include "offchip_pred_base.h"
#include "hmp_commons.h"

class OffchipPredHMPGshare : public OffchipPredBase
{
    private:
        uint32_t GHR;
        vector<confidence_state_t> PHT;
        
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
                uint64_t strong_nt_to_weak_nt;
                uint64_t strong_nt_to_strong_nt;
                uint64_t weak_nt_to_weak_t;
                uint64_t weak_nt_to_strong_nt;
                uint64_t weak_t_to_strong_t;
                uint64_t weak_t_to_weak_nt;
                uint64_t strong_t_to_strong_t;
                uint64_t strong_t_to_weak_t;
            } train;
        } stats;

    private:
        uint32_t get_hash(uint64_t pc);

    public:
        OffchipPredHMPGshare(uint32_t cpu, string _type, uint64_t _seed);
        ~OffchipPredHMPGshare();
        
        /* interface functions */
        void print_config();
        void dump_stats();
        void reset_stats();
        void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
        bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
};

#endif /* OFFCHIP_PRED_HMP_GSHARE_H */

