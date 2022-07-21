#ifndef OFFCHIP_PRED_BASE_H
#define OFFCHIP_PRED_BASE_H

#include "block.h"
#include "offchip_pred_base_helper.h"
#include <string>

class OffchipPredBase
{
public:
    uint32_t cpu;
    string type;
    uint64_t seed;
    uint8_t dram_bw; // current DRAM bandwidth bucket

    OffchipPredBase(uint32_t _cpu, string _type, uint64_t _seed) : cpu(_cpu), type(_type), seed(_seed) 
    {
        srand(seed);
        dram_bw = 0;
    }
    ~OffchipPredBase() {}
    void update_dram_bw(uint8_t _dram_bw) { dram_bw = _dram_bw; }

    virtual void print_config();
    virtual void dump_stats();
    virtual void reset_stats();
    virtual void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
    virtual bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
};

#endif /* OFFCHIP_PRED_BASE_H */


