/*****************************************************************************************
 * This file implements the the ensemble version of Hit-miss predictor (HMP) introduced in:
 * Yoaz, A., Erez, M., Ronen, R., & Jourdan, S., 
 * "Speculation techniques for improving load related instruction scheduling", ISCA 1999
 * https://doi.org/10.1145/307338.300983
 *****************************************************************************************
 */

#ifndef OFFCHIP_PRED_HMP_ENSEMBLE_H
#define OFFCHIP_PRED_HMP_ENSEMBLE_H

#include "offchip_pred_base.h"
#include "hmp_commons.h"
#include "offchip_pred_hmp_local.h"
#include "offchip_pred_hmp_gshare.h"
#include "offchip_pred_hmp_gskew.h"

class OffchipPredHMPEnsemble : public OffchipPredBase
{
    private:
        OffchipPredHMPLocal*  hmp_local;
        OffchipPredHMPGshare* hmp_gshare;
        OffchipPredHMPGskew*  hmp_gskew;

    private:

    public:
        OffchipPredHMPEnsemble(uint32_t cpu, string _type, uint64_t _seed);
        ~OffchipPredHMPEnsemble();
        
        /* interface functions */
        void print_config();
        void dump_stats();
        void reset_stats();
        void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
        bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
};


#endif /* OFFCHIP_PRED_HMP_ENSEMBLE_H */

