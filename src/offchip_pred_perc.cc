#include <iostream>
#include <algorithm>
#include "offchip_pred_perc.h"
#include "util.h"
#include "ooo_cpu.h"

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
    extern vector<int32_t> ocp_perc_activated_features;
    extern vector<int32_t> ocp_perc_weight_array_sizes;
    extern vector<int32_t> ocp_perc_feature_hash_types;
    extern float ocp_perc_activation_threshold;
    extern float ocp_perc_max_weight;
    extern float ocp_perc_min_weight;
    extern float ocp_perc_pos_weight_delta;
    extern float ocp_perc_neg_weight_delta;
    extern float ocp_perc_pos_train_thresh;
    extern float ocp_perc_neg_train_thresh;
    extern uint32_t ocp_perc_page_buf_sets;
    extern uint32_t ocp_perc_page_buf_assoc;
    extern uint32_t ocp_perc_last_n_load_pcs;
    extern uint32_t ocp_perc_last_n_pcs;
    extern bool ocp_perc_enable_dynamic_act_thresh;
    extern uint32_t ocp_perc_update_act_thresh_epoch;
    extern uint32_t ocp_perc_high_critical_dram_bw_level;
    extern uint32_t ocp_perc_low_critical_dram_bw_level;
    extern float ocp_perc_poor_precision_thresh;
    extern float ocp_perc_act_thresh_update_gradient;
    extern float ocp_perc_min_activation_threshold;
    extern float ocp_perc_max_activation_threshold;
}

void OffchipPredPerc::print_config()
{
    cout << "ocp_perc_activated_features " << print_activated_features(knob::ocp_perc_activated_features) << endl
         << "ocp_perc_weight_array_sizes " << array_to_string(knob::ocp_perc_weight_array_sizes) << endl
         << "ocp_perc_feature_hash_types " << array_to_string(knob::ocp_perc_feature_hash_types) << endl
         << "ocp_perc_activation_threshold " << knob::ocp_perc_activation_threshold << endl
         << "ocp_perc_max_weight " << knob::ocp_perc_max_weight << endl
         << "ocp_perc_min_weight " << knob::ocp_perc_min_weight << endl
         << "ocp_perc_pos_weight_delta " << knob::ocp_perc_pos_weight_delta << endl
         << "ocp_perc_neg_weight_delta " << knob::ocp_perc_neg_weight_delta << endl
         << "ocp_perc_pos_train_thresh " << knob::ocp_perc_pos_train_thresh << endl
         << "ocp_perc_neg_train_thresh " << knob::ocp_perc_neg_train_thresh << endl
         << "ocp_perc_page_buf_sets " << knob::ocp_perc_page_buf_sets << endl
         << "ocp_perc_page_buf_assoc " << knob::ocp_perc_page_buf_assoc << endl
         << "ocp_perc_last_n_load_pcs " << knob::ocp_perc_last_n_load_pcs << endl
         << "ocp_perc_last_n_pcs " << knob::ocp_perc_last_n_pcs << endl
         << "ocp_perc_enable_dynamic_act_thresh " << knob::ocp_perc_enable_dynamic_act_thresh << endl
         << "ocp_perc_update_act_thresh_epoch " << knob::ocp_perc_update_act_thresh_epoch << endl
         << "ocp_perc_high_critical_dram_bw_level " << knob::ocp_perc_high_critical_dram_bw_level << endl
         << "ocp_perc_low_critical_dram_bw_level " << knob::ocp_perc_low_critical_dram_bw_level << endl
         << "ocp_perc_poor_precision_thresh " << knob::ocp_perc_poor_precision_thresh << endl
         << "ocp_perc_act_thresh_update_gradient " << knob::ocp_perc_act_thresh_update_gradient << endl
         << "ocp_perc_min_activation_threshold " << knob::ocp_perc_min_activation_threshold << endl
         << "ocp_perc_max_activation_threshold " << knob::ocp_perc_max_activation_threshold << endl
         << endl;
}

void OffchipPredPerc::dump_stats()
{
    cout << "ocp_perc_train_called " << stats.train.called << endl
         << "ocp_perc_predict_called " << stats.predict.called << endl
         << "ocp_perc_predict_offchip " << stats.predict.outcome[1] << endl
         << "ocp_perc_predict_not_offchip " << stats.predict.outcome[0] << endl
         << endl
         << "ocp_perc_unique_pages " << unique_pages.size() << endl
         << "ocp_perc_page_buf_lookup_called " << stats.page_buf.called << endl
         << "ocp_perc_page_buf_lookup_hit " << stats.page_buf.hit << endl
         << "ocp_perc_page_buf_lookup_eviction " << stats.page_buf.eviction << endl
         << "ocp_perc_page_buf_lookup_insertion " << stats.page_buf.insertion << endl
         << endl
         << "ocp_perc_act_thresh_update_called " << stats.act_thresh_update.called << endl
         << "ocp_perc_act_thresh_update_increment " << stats.act_thresh_update.increment << endl
         << "ocp_perc_act_thresh_update_decrement " << stats.act_thresh_update.decrement << endl
         << "ocp_perc_act_thresh_update_max_observed_thresh " << stats.act_thresh_update.max_observed_thresh << endl
         << "ocp_perc_act_thresh_update_min_observed_thresh " << stats.act_thresh_update.min_observed_thresh << endl
         << endl;

    perc_pred->dump_stats();
}

void OffchipPredPerc::reset_stats()
{
    bzero(&stats, sizeof(stats));
    stats.act_thresh_update.max_observed_thresh = -999999999;
    stats.act_thresh_update.min_observed_thresh = 999999999;
    perc_pred->reset_stats();
}

OffchipPredPerc::OffchipPredPerc(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    perc_pred = new perceptron_pred_t(knob::ocp_perc_activated_features, 
                                      knob::ocp_perc_weight_array_sizes,
                                      knob::ocp_perc_feature_hash_types, 
                                      knob::ocp_perc_activation_threshold, 
                                      knob::ocp_perc_max_weight,
                                      knob::ocp_perc_min_weight,
                                      knob::ocp_perc_pos_weight_delta, 
                                      knob::ocp_perc_neg_weight_delta,
                                      knob::ocp_perc_pos_train_thresh,
                                      knob::ocp_perc_neg_train_thresh
                                      );
    perc_pred->set_cpu(cpu);

    // init page buffer
    for(uint32_t index = 0; index < knob::ocp_perc_page_buf_sets; ++index)
    {
        deque<ocp_perc_page_buf_entry_t*> d;
        d.clear();
        m_page_buffer.push_back(d);
    }
    true_pos = 0;
    false_pos = 0;
    false_neg = 0;
    true_neg = 0;
    train_count = 0;

    reset_stats();
}

OffchipPredPerc::~OffchipPredPerc()
{

}

bool OffchipPredPerc::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    state_info_t *info = get_state(arch_instr, data_index, lq_entry);
    float perc_weight_sum = 0.0;
    bool prediction = false;

    // get prediction
    perc_pred->predict(info, prediction, perc_weight_sum);

    // save all necessary data that would 
    // later be required for training in LQ entry
    ocp_perc_feature_t *feature = new ocp_perc_feature_t();
    feature->info = info;
    feature->perc_weight_sum = perc_weight_sum;
    lq_entry->ocp_feature = feature;

    stats.predict.called++;
    stats.predict.outcome[prediction]++;

    return prediction;
}

void OffchipPredPerc::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    train_count++;

    // keep track of true/false positives/negatives
    if(lq_entry->went_offchip_pred && lq_entry->went_offchip)           true_pos++;
    else if(lq_entry->went_offchip_pred && !lq_entry->went_offchip)     false_pos++;
    else if(!lq_entry->went_offchip_pred && lq_entry->went_offchip)     false_neg++;
    else if(!lq_entry->went_offchip_pred && !lq_entry->went_offchip)    true_neg++;

    // check if an update to activation threshold is needed
    if(knob::ocp_perc_enable_dynamic_act_thresh && train_count % knob::ocp_perc_update_act_thresh_epoch == 0)
    {
        check_and_update_act_thresh();
    }

    // retreive all necessary data from LQ entry
    // that were used before for prediction making
    ocp_perc_feature_t *feature = (ocp_perc_feature_t*)lq_entry->ocp_feature;
    state_info_t *info = feature->info;
    float perc_weight_sum = feature->perc_weight_sum;

    // train perceptron
    perc_pred->train(info, perc_weight_sum, lq_entry->went_offchip_pred, lq_entry->went_offchip);

    stats.train.called++;
}

state_info_t* OffchipPredPerc::get_state(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    uint64_t load_pc = arch_instr->ip;
    uint64_t vaddr = lq_entry->virtual_address;
    uint64_t vpage = vaddr >> LOG2_PAGE_SIZE;
    uint32_t voffset = (vaddr >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
    uint32_t v_cl_offset = vaddr & ((1ull << LOG2_BLOCK_SIZE) - 1);
    uint32_t v_cl_word_offset = v_cl_offset >> 2;
    uint32_t v_cl_dword_offset = v_cl_offset >> 4;

    state_info_t *info = new state_info_t();

    // get control-flow features
    uint64_t last_n_load_pc_sig = 0, last_n_pc_sig = 0;
    get_control_flow_signatures(lq_entry, last_n_load_pc_sig, last_n_pc_sig);

    // get data-flow features
    bool first_access = false;
    lookup_address(vaddr, vpage, voffset, first_access);

    // populate features
    info->pc = load_pc;
    info->last_n_load_pc_sig = last_n_load_pc_sig;
    info->last_n_pc_sig = last_n_pc_sig;
    info->data_index = data_index;
    info->vaddr = vaddr;
    info->vpage = vpage;
    info->voffset = voffset;
    info->first_access = first_access;
    info->v_cl_offset = v_cl_offset;
    info->v_cl_word_offset = v_cl_word_offset;
    info->v_cl_dword_offset = v_cl_dword_offset;

    return info;
    // return NULL;
}

void OffchipPredPerc::lookup_address(uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool &first_access)
{
    stats.page_buf.called++;
    unique_pages.insert(vpage);

    ocp_perc_page_buf_entry_t *entry = NULL;
    uint32_t set = get_set(vpage);
    auto it = find_if(m_page_buffer[set].begin(), m_page_buffer[set].end(), [vpage](ocp_perc_page_buf_entry_t *entry)
                      { return entry->page == vpage; });

    if (it != m_page_buffer[set].end()) // page hit
    {
        entry = (*it);
        first_access = !entry->bmp_access.test(voffset);
        entry->bmp_access.set(voffset);
        entry->age = 0;
        m_page_buffer[set].erase(it);
        m_page_buffer[set].push_back(entry);
        stats.page_buf.hit++;
    }
    else
    {
        if (m_page_buffer[set].size() >= knob::ocp_perc_page_buf_assoc)
        {
            entry = m_page_buffer[set].front();
            m_page_buffer[set].pop_front();
            stats.page_buf.eviction++;
            delete entry;
        }

        entry = new ocp_perc_page_buf_entry_t();
        entry->page = vpage;
        entry->bmp_access.set(voffset);
        entry->age = 0;
        m_page_buffer[set].push_back(entry);
        first_access = true;
        stats.page_buf.insertion++;
    }
}

uint32_t OffchipPredPerc::get_set(uint64_t page)
{
    uint32_t hash = HashZoo::fnv1a64(page);
    return hash % knob::ocp_perc_page_buf_sets;
}

void OffchipPredPerc::get_control_flow_signatures(LSQ_ENTRY *lq_entry, uint64_t &last_n_load_pc_sig, uint64_t &last_n_pc_sig)
{
    // signature from last N load PCs
    uint64_t curr_pc = lq_entry->ip;
    if(last_n_load_pcs.size() >= knob::ocp_perc_last_n_load_pcs)
    {
        last_n_load_pcs.pop_front();
    }
    last_n_load_pcs.push_back(curr_pc);

    last_n_load_pc_sig = 0;
    for(uint32_t index = 0; index < last_n_load_pcs.size(); ++index)
    {
        last_n_load_pc_sig <<= 1;
        last_n_load_pc_sig ^= last_n_load_pcs[index];
    }

    // signature from last N instruction PCs
    deque<uint64_t> last_n_pcs;
    int prior = lq_entry->rob_index;
    for(int i = 0; i < (int)knob::ocp_perc_last_n_pcs-1; ++i)
    {
        last_n_pcs.push_front(ooo_cpu[cpu].ROB.entry[prior].ip);
        prior--;
        if(prior < 0)
            prior = ooo_cpu[cpu].ROB.SIZE - 1;
    }

    last_n_pc_sig = 0;
    for(uint32_t index = 0; index < last_n_pcs.size(); ++index)
    {
        last_n_pc_sig <<= 1;
        last_n_pc_sig ^= last_n_pcs[index];
    }
}

string OffchipPredPerc::print_activated_features(vector<int32_t> activated_features)
{
    std::stringstream ss;
    for (uint32_t feature = 0; feature < activated_features.size(); ++feature)
    {
        if (feature)
            ss << ",";
        ss << perc::feature_names[activated_features[feature]];
    }
    return ss.str();
}

void OffchipPredPerc::check_and_update_act_thresh()
{
    stats.act_thresh_update.called++;
    float precision = (float)true_pos/(true_pos+false_pos);
    
    // if the DRAM bandwidth consumption is high AND the precision of the predictor is low,
    // then try to improve the precision by increasing activation threshold
    if(dram_bw >= knob::ocp_perc_high_critical_dram_bw_level && precision <= knob::ocp_perc_poor_precision_thresh)
    {
        stats.act_thresh_update.increment++;
        float old_thresh = perc_pred->get_activation_threshold();
        float new_thresh = min((old_thresh + knob::ocp_perc_act_thresh_update_gradient), knob::ocp_perc_max_activation_threshold);
        perc_pred->set_activation_threshold(new_thresh);

        MYLOG(warmup_complete[cpu], "incr_act dram_bw: %d true_pos: %ld false_pos: %ld false_neg: %ld true_neg: %ld precision: %f recall: %f old_act_thresh: %f new_act_thresh: %f", +dram_bw, true_pos, false_pos, false_neg, true_neg, precision, recall, old_thresh, new_thresh);
        
        // stats collect
        if(new_thresh < stats.act_thresh_update.min_observed_thresh) stats.act_thresh_update.min_observed_thresh = new_thresh;
        if(new_thresh > stats.act_thresh_update.max_observed_thresh) stats.act_thresh_update.max_observed_thresh = new_thresh;
    }
    else if(dram_bw <= knob::ocp_perc_low_critical_dram_bw_level)
    {
        stats.act_thresh_update.decrement++;
        float old_thresh = perc_pred->get_activation_threshold();
        float new_thresh = max((old_thresh - knob::ocp_perc_act_thresh_update_gradient), knob::ocp_perc_min_activation_threshold);
        perc_pred->set_activation_threshold(new_thresh);
        
        MYLOG(warmup_complete[cpu], "decr_act dram_bw: %d true_pos: %ld false_pos: %ld false_neg: %ld true_neg: %ld precision: %f recall: %f old_act_thresh: %f new_act_thresh: %f", +dram_bw, true_pos, false_pos, false_neg, true_neg, precision, recall, old_thresh, new_thresh);
        
        // stats collect
        if(new_thresh < stats.act_thresh_update.min_observed_thresh) stats.act_thresh_update.min_observed_thresh = new_thresh;
        if(new_thresh > stats.act_thresh_update.max_observed_thresh) stats.act_thresh_update.max_observed_thresh = new_thresh;
    }
    else
    {
        MYLOG(warmup_complete[cpu], "act_same dram_bw: %d true_pos: %ld false_pos: %ld false_neg: %ld true_neg: %ld precision: %f recall: %f", +dram_bw, true_pos, false_pos, false_neg, true_neg, precision, recall);
    }
}