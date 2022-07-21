#include <iostream>
#include "offchip_pred_hmp_ensemble.h"
#include "util.h"

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
#define MYLOG(cond, ...) \
    {                    \
    }
#endif

namespace knob
{

}

void OffchipPredHMPEnsemble::print_config()
{
    hmp_local->print_config();
    hmp_gshare->print_config();
    hmp_gskew->print_config();
}

void OffchipPredHMPEnsemble::dump_stats()
{
    hmp_local->dump_stats();
    hmp_gshare->dump_stats();
    hmp_gskew->dump_stats();
}

void OffchipPredHMPEnsemble::reset_stats()
{
    hmp_local->reset_stats();
    hmp_gshare->reset_stats();
    hmp_gskew->reset_stats();
}

OffchipPredHMPEnsemble::OffchipPredHMPEnsemble(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    hmp_local  = new OffchipPredHMPLocal(_cpu, _type, _seed);
    hmp_gshare = new OffchipPredHMPGshare(_cpu, _type, _seed);
    hmp_gskew  = new OffchipPredHMPGskew(_cpu, _type, _seed);
}

OffchipPredHMPEnsemble::~OffchipPredHMPEnsemble()
{

}

bool OffchipPredHMPEnsemble::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    bool pred_hmp_local  = hmp_local->predict(arch_instr, data_index, lq_entry);
    bool pred_hmp_gshare = hmp_gshare->predict(arch_instr, data_index, lq_entry);
    bool pred_hmp_gskew  = hmp_gskew->predict(arch_instr, data_index, lq_entry);

    uint32_t voted_pos = 0;
    if(pred_hmp_local) voted_pos++;
    if(pred_hmp_gshare) voted_pos++;
    if(pred_hmp_gskew) voted_pos++;

    // majority
    return (voted_pos >= 2);
}

void OffchipPredHMPEnsemble::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    hmp_local->train(arch_instr, data_index, lq_entry);
    hmp_gshare->train(arch_instr, data_index, lq_entry);
    hmp_gskew->train(arch_instr, data_index, lq_entry);
}