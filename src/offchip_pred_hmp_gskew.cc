#include <iostream>
#include "offchip_pred_hmp_gskew.h"
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
    extern uint32_t ocp_hmp_gskew_history_length;
    extern uint32_t ocp_hmp_gskew_num_hashes;
    extern vector<int32_t> ocp_hmp_gskew_hash_types;
    extern uint32_t ocp_hmp_gskew_pht_size;
}

void OffchipPredHMPGskew::print_config()
{
    cout << "ocp_hmp_gskew_history_length " << knob::ocp_hmp_gskew_history_length << endl
        << "ocp_hmp_gskew_num_hashes " << knob::ocp_hmp_gskew_num_hashes << endl
        << "ocp_hmp_gskew_hash_types " << array_to_string(knob::ocp_hmp_gskew_hash_types) << endl        
        << "ocp_hmp_gskew_pht_size " << knob::ocp_hmp_gskew_pht_size << endl
        << endl;
}

void OffchipPredHMPGskew::dump_stats()
{
    cout << "ocp_hmp_gskew_predict_called " << stats.predict.called << endl
         << endl
         << "ocp_hmp_gskew_train_called " << stats.train.called << endl
         ;
    for(uint32_t index = 0; index < knob::ocp_hmp_gskew_num_hashes; ++index)
    {
        cout << "ocp_hmp_gskew_train_strong_nt_to_weak_nt_ " << index << " " << stats.train.strong_nt_to_weak_nt[index] << endl
            << "ocp_hmp_gskew_train_strong_nt_to_strong_nt_ " << index << " " << stats.train.strong_nt_to_strong_nt[index] << endl
            << "ocp_hmp_gskew_train_weak_nt_to_weak_t_ " << index << " " << stats.train.weak_nt_to_weak_t[index] << endl
            << "ocp_hmp_gskew_train_weak_nt_to_strong_nt_ " << index << " " << stats.train.weak_nt_to_strong_nt[index] << endl
            << "ocp_hmp_gskew_train_weak_t_to_strong_t_ " << index << " " << stats.train.weak_t_to_strong_t[index] << endl
            << "ocp_hmp_gskew_train_weak_t_to_weak_nt_ " << index << " " << stats.train.weak_t_to_weak_nt[index] << endl
            << "ocp_hmp_gskew_train_strong_t_to_strong_t_ " << index << " " << stats.train.strong_t_to_strong_t[index] << endl
            << "ocp_hmp_gskew_train_strong_t_to_weak_t_ " << index << " " << stats.train.strong_t_to_weak_t[index] << endl;
    }
    cout << endl;
}

void OffchipPredHMPGskew::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredHMPGskew::OffchipPredHMPGskew(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    assert(knob::ocp_hmp_gskew_hash_types.size() == knob::ocp_hmp_gskew_num_hashes);

    // Init GHR
    GHR = 0x0;

    // Init PHT
    vector<confidence_state_t> v;
    PHT.resize(knob::ocp_hmp_gskew_num_hashes, v);
    for(uint32_t index = 0; index < PHT.size(); ++index)
    {
        PHT[index].resize(knob::ocp_hmp_gskew_pht_size, WEAK_T);
    }
}

OffchipPredHMPGskew::~OffchipPredHMPGskew()
{

}

bool OffchipPredHMPGskew::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.predict.called++;

    uint64_t pc = arch_instr->ip;
    vector<uint32_t> hashed_indices = get_hashes(pc);
    assert(hashed_indices.size() == knob::ocp_hmp_gskew_num_hashes); 

    vector<bool> predictions;
    predictions.resize(knob::ocp_hmp_gskew_num_hashes, false);
    uint32_t voted_pos = 0, voted_neg = 0;

    for(uint32_t index = 0; index < knob::ocp_hmp_gskew_num_hashes; ++index)
    {
        predictions[index] = (PHT[index][hashed_indices[index]] >= WEAK_T) ? true : false;
        if(predictions[index]) 
            voted_pos++; 
        else
            voted_neg++;
    }

    // majority vote
    return (voted_pos >= voted_neg) ? true : false;
}

void OffchipPredHMPGskew::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    stats.train.called++;

    uint64_t pc = arch_instr->ip;
    bool went_offchip = lq_entry->went_offchip;
    vector<uint32_t> hashed_indices = get_hashes(pc);
    assert(hashed_indices.size() == knob::ocp_hmp_gskew_num_hashes);

    for(uint32_t index = 0; index < knob::ocp_hmp_gskew_num_hashes; ++index)
    {
        PHT[index][hashed_indices[index]] = update_confidence_counter(index, PHT[index][hashed_indices[index]], went_offchip);
    }

    // update global history
    GHR = GHR << 1;
    if(went_offchip) GHR += 1;
    GHR &= ((1u << knob::ocp_hmp_gskew_history_length) - 1);
}

vector<uint32_t> OffchipPredHMPGskew::get_hashes(uint64_t pc)
{
    uint32_t folded_pc = folded_xor(pc, 2);
    vector<uint32_t> hashed_indices;
    hashed_indices.resize(knob::ocp_hmp_gskew_num_hashes);
    for(uint32_t index = 0; index < knob::ocp_hmp_gskew_num_hashes; ++index)
    {
        uint32_t tmp = folded_pc ^ GHR;
        tmp = HashZoo::getHash(knob::ocp_hmp_gskew_hash_types[index], tmp);
        hashed_indices[index] = tmp % knob::ocp_hmp_gskew_pht_size;
    }

    return hashed_indices;
}

confidence_state_t OffchipPredHMPGskew::update_confidence_counter(uint32_t hash_id, confidence_state_t old_conf, bool went_offchip)
{
    confidence_state_t new_conf = WEAK_NT;
    if(old_conf == STRONG_NT)
    {
        new_conf = went_offchip ? WEAK_NT : STRONG_NT;
        if(new_conf == WEAK_NT) stats.train.strong_nt_to_weak_nt[hash_id]++; else stats.train.strong_nt_to_strong_nt[hash_id]++;
    }
    else if(old_conf == WEAK_NT)
    {
        new_conf = went_offchip ? WEAK_T : STRONG_NT;
        if(new_conf == WEAK_T) stats.train.weak_nt_to_weak_t[hash_id]++; else stats.train.weak_nt_to_strong_nt[hash_id]++;
    }
    else if(old_conf == WEAK_T)
    {
        new_conf = went_offchip ? STRONG_T : WEAK_NT;
        if(new_conf == STRONG_T) stats.train.weak_t_to_strong_t[hash_id]++; else stats.train.weak_t_to_weak_nt[hash_id]++;
    }
    else if(old_conf == STRONG_T)
    {
        new_conf = went_offchip ? STRONG_T : WEAK_T;
        if(new_conf == STRONG_T) stats.train.strong_t_to_strong_t[hash_id]++; else stats.train.strong_t_to_weak_t[hash_id]++;
    }
    else
    {
        assert(false);
    }
    return new_conf;
}