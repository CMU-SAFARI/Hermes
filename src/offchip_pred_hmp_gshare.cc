#include <iostream>
#include "offchip_pred_hmp_gshare.h"
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
    extern uint32_t ocp_hmp_gshare_history_length;
    extern uint32_t ocp_hmp_gshare_pc_hash_type;
}

void OffchipPredHMPGshare::print_config()
{
    cout << "ocp_hmp_gshare_history_length " << knob::ocp_hmp_gshare_history_length << endl
        << "ocp_hmp_gshare_pc_hash_type " << knob::ocp_hmp_gshare_pc_hash_type << endl
        << endl;
}

void OffchipPredHMPGshare::dump_stats()
{
    cout << "ocp_hmp_gshare_predict_called " << stats.predict.called << endl
         << endl
         << "ocp_hmp_gshare_train_called " << stats.train.called << endl
         << "ocp_hmp_gshare_train_strong_nt_to_weak_nt " << stats.train.strong_nt_to_weak_nt << endl
         << "ocp_hmp_gshare_train_strong_nt_to_strong_nt " << stats.train.strong_nt_to_strong_nt << endl
         << "ocp_hmp_gshare_train_weak_nt_to_weak_t " << stats.train.weak_nt_to_weak_t << endl
         << "ocp_hmp_gshare_train_weak_nt_to_strong_nt " << stats.train.weak_nt_to_strong_nt << endl
         << "ocp_hmp_gshare_train_weak_t_to_strong_t " << stats.train.weak_t_to_strong_t << endl
         << "ocp_hmp_gshare_train_weak_t_to_weak_nt " << stats.train.weak_t_to_weak_nt << endl
         << "ocp_hmp_gshare_train_strong_t_to_strong_t " << stats.train.strong_t_to_strong_t << endl
         << "ocp_hmp_gshare_train_strong_t_to_weak_t " << stats.train.strong_t_to_weak_t << endl
         << endl;
}

void OffchipPredHMPGshare::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredHMPGshare::OffchipPredHMPGshare(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    GHR = 0x0;
    PHT.resize((1u << knob::ocp_hmp_gshare_history_length), WEAK_T);
}

OffchipPredHMPGshare::~OffchipPredHMPGshare()
{

}

bool OffchipPredHMPGshare::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.predict.called++;

    bool prediction = false;
    uint64_t pc = arch_instr->ip;
    uint32_t hashed_pc = get_hash(pc);

    /* First, XOR the global history with hashed PC */
    uint32_t pht_index = GHR ^ hashed_pc;
    pht_index = pht_index & ((1u << knob::ocp_hmp_gshare_history_length) - 1);

    /* Second, retrieve confidence counter from PHT */
    confidence_state_t conf_state = PHT[pht_index];

    /* Third, make the prediction */
    if(conf_state >= WEAK_T) prediction = true;

    return prediction;
}

void OffchipPredHMPGshare::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.train.called++;

    uint64_t pc = arch_instr->ip;
    uint32_t hashed_pc = get_hash(pc);
    bool went_offchip = lq_entry->went_offchip;

    /* First, XOR the global history with hashed PC */
    uint32_t pht_index = GHR ^ hashed_pc;
    pht_index = pht_index & ((1u << knob::ocp_hmp_gshare_history_length) - 1);

    /* Second, update confidence counter in PHT */
    confidence_state_t old_conf = PHT[pht_index], new_conf = WEAK_NT;
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
        cout << "Invalid confidence found at PHT index " << pht_index << endl;
        assert(false);
    }
    PHT[pht_index] = new_conf;

    /* Third, update global history */
    GHR = GHR << 1;
    if(went_offchip) GHR += 1;
    GHR &= ((1u << knob::ocp_hmp_gshare_history_length) - 1);
}

uint32_t OffchipPredHMPGshare::get_hash(uint64_t pc)
{
    uint32_t folded_pc = folded_xor(pc, 2);
    uint32_t hashed_pc = HashZoo::getHash(knob::ocp_hmp_gshare_pc_hash_type, folded_pc);
    hashed_pc = hashed_pc & ((1u << knob::ocp_hmp_gshare_history_length) - 1);
    return hashed_pc;
}