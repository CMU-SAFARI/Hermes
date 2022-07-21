#ifndef OFFCHIP_PRED_RANDOM_H
#define OFFCHIP_PRED_RANDOM_H

#include <random>
#include "offchip_pred_base.h"

class OffchipPredRandom : public OffchipPredBase
{
    private:
        std::default_random_engine generator;
        std::bernoulli_distribution *selector;

        struct
        {
            struct
            {
                uint64_t called;
                uint64_t pos;
                uint64_t neg;
            } pred;
        } stats;

    public:
        OffchipPredRandom(uint32_t _cpu, string _type, uint64_t _seed);
        ~OffchipPredRandom();

        void print_config();
        void dump_stats();
        void reset_stats();
        void train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
        bool predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry);
};

#endif /* OFFCHIP_PRED_RANDOM_H */


