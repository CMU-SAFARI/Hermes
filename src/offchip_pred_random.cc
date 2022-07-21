#include <iostream>
#include "offchip_pred_random.h"

namespace knob
{
    extern float ocp_random_pos_rate;
}

void OffchipPredRandom::print_config()
{
    cout << "ocp_random_pos_rate " << knob::ocp_random_pos_rate
         << endl;
}

void OffchipPredRandom::dump_stats()
{
    cout << "ocp_random_predict_called " << stats.pred.called << endl
         << "ocp_random_predict_pos " << stats.pred.pos << endl
         << "ocp_random_predict_neg " << stats.pred.neg << endl
         << endl;
}

void OffchipPredRandom::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredRandom::OffchipPredRandom(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    assert(knob::ocp_random_pos_rate <= 1.0);
    generator.seed(seed);
	selector = new std::bernoulli_distribution(knob::ocp_random_pos_rate);
}

OffchipPredRandom::~OffchipPredRandom()
{

}

void OffchipPredRandom::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    // nothing to train
}

bool OffchipPredRandom::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.pred.called++;

    // predict randomly
    bool pred = (*selector)(generator);
    if(pred)
        stats.pred.pos++;
    else
        stats.pred.neg++;
    return pred;
}