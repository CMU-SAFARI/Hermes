#include <iostream>
#include "offchip_pred_hmp_local.h"
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
    extern uint32_t ocp_hmp_local_history_length;
    extern uint32_t ocp_hmp_local_lhr_size;
    extern uint32_t ocp_hmp_local_lhr_index_hash_type;
}

void OffchipPredHMPLocal::print_config()
{
    cout << "ocp_hmp_local_history_length " << knob::ocp_hmp_local_history_length << endl
         << "ocp_hmp_local_lhr_size " << knob::ocp_hmp_local_lhr_size << endl
         << "ocp_hmp_local_lhr_index_hash_type " << knob::ocp_hmp_local_lhr_index_hash_type << endl
         << endl;
}

void OffchipPredHMPLocal::dump_stats()
{
    cout << "ocp_hmp_local_predict_called " << stats.predict.called << endl
         << endl
         << "ocp_hmp_local_train_called " << stats.train.called << endl
         << "ocp_hmp_local_train_strong_nt_to_weak_nt " << stats.train.strong_nt_to_weak_nt << endl
         << "ocp_hmp_local_train_strong_nt_to_strong_nt " << stats.train.strong_nt_to_strong_nt << endl
         << "ocp_hmp_local_train_weak_nt_to_weak_t " << stats.train.weak_nt_to_weak_t << endl
         << "ocp_hmp_local_train_weak_nt_to_strong_nt " << stats.train.weak_nt_to_strong_nt << endl
         << "ocp_hmp_local_train_weak_t_to_strong_t " << stats.train.weak_t_to_strong_t << endl
         << "ocp_hmp_local_train_weak_t_to_weak_nt " << stats.train.weak_t_to_weak_nt << endl
         << "ocp_hmp_local_train_strong_t_to_strong_t " << stats.train.strong_t_to_strong_t << endl
         << "ocp_hmp_local_train_strong_t_to_weak_t " << stats.train.strong_t_to_weak_t << endl
         << endl;
}

void OffchipPredHMPLocal::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredHMPLocal::OffchipPredHMPLocal(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    LHR.resize(knob::ocp_hmp_local_lhr_size, 0);
    PHT.resize((1u << knob::ocp_hmp_local_history_length), WEAK_T);
}

OffchipPredHMPLocal::~OffchipPredHMPLocal()
{

}

bool OffchipPredHMPLocal::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.predict.called++;
    bool prediction = false;
    uint64_t pc = arch_instr->ip;
    uint32_t lhr_index = get_index(pc);
    
    /* First, get local history of this load PC */
    uint32_t local_history = LHR[lhr_index];

    /* Second, use the history to retreive confidence counter from PHT */
    assert(local_history < (1u << knob::ocp_hmp_local_history_length));
    confidence_state_t conf_state = PHT[local_history];

    /* Third, make the prediction */
    if(conf_state >= WEAK_T) prediction = true;

    MYLOG(warmup_complete[cpu], "pc %lx lhr_index %d local_history %x conf_state %d", pc, lhr_index, local_history, conf_state);

    return prediction;
}

void OffchipPredHMPLocal::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.train.called++;

    uint64_t pc = arch_instr->ip;
    bool went_offchip = lq_entry->went_offchip;
    uint32_t lhr_index = get_index(pc);
    uint32_t old_local_history = LHR[lhr_index];
    uint32_t old_conf =  PHT[old_local_history];

    /* First, update the confidence counter in PHT */
    confidence_state_t new_conf = WEAK_NT;
    if(old_conf == STRONG_NT)
    {
        new_conf = went_offchip ? WEAK_NT : STRONG_NT;
        if(new_conf == WEAK_NT) stats.train.strong_nt_to_weak_nt++; else stats.train.strong_nt_to_strong_nt++;
    }
    else if(old_conf == WEAK_NT)
    {
        new_conf = went_offchip ? WEAK_T : STRONG_NT;
        if(new_conf == WEAK_T) stats.train.weak_nt_to_weak_t++; else stats.train.weak_nt_to_strong_nt++;
    }
    else if(old_conf == WEAK_T)
    {
        new_conf = went_offchip ? STRONG_T : WEAK_NT;
        if(new_conf == STRONG_T) stats.train.weak_t_to_strong_t++; else stats.train.weak_t_to_weak_nt++;
    }
    else if(old_conf == STRONG_T)
    {
        new_conf = went_offchip ? STRONG_T : WEAK_T;
        if(new_conf == STRONG_T) stats.train.strong_t_to_strong_t++; else stats.train.strong_t_to_weak_t++;
    }
    else
    {
        cout << "Invalid confidence found at PHT index " << old_local_history << endl;
        assert(false);
    }
    PHT[old_local_history] = new_conf;

    /* Second, update local history */
    uint32_t new_local_history = 0;
    new_local_history = old_local_history << 1;
    if(went_offchip) new_local_history += 1;
    new_local_history = new_local_history & ((1u << knob::ocp_hmp_local_history_length) - 1);
    LHR[lhr_index] = new_local_history;

    MYLOG(warmup_complete[cpu], "pc %lx lhr_index %d went_offchip %d old_conf %d new_conf %d old_local_history %x new_local_history %x", pc, lhr_index, +went_offchip, old_conf, new_conf, old_local_history, new_local_history);
}

uint32_t OffchipPredHMPLocal::get_index(uint64_t pc)
{
    uint32_t pc_folded_xor = folded_xor(pc, 2);
    uint32_t hash = HashZoo::getHash(knob::ocp_hmp_local_lhr_index_hash_type, pc_folded_xor);
    return (hash % knob::ocp_hmp_local_lhr_size);
}