#include <iostream>
#include "offchip_pred_base.h"

void OffchipPredBase::print_config()
{

}

void OffchipPredBase::dump_stats()
{

}

void OffchipPredBase::reset_stats()
{

}

void OffchipPredBase::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    // nothing to train
}

bool OffchipPredBase::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    // predict randomly
    // return (rand() % 2) ? true : false;
    return false;
}