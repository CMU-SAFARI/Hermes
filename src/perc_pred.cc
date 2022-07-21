#include <stdio.h>
#include <iomanip>
#include <assert.h>
#include "perc_pred.h"
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

namespace perc
{

string feature_names[] = {
    "PC",
    "Offset",
    "Page",
    "Addr",
    "FirstAccess",
    "PC_Offset",
    "PC_Page",
    "PC_Addr",
    "PC_FirstAccess",
    "Offset_FirstAccess",
    "CLOffset",
    "PC_CLOffset",
    "CLWordOffset",
    "PC_CLWordOffset",
    "CLDWordOffset",
    "PC_CLDWordOffset",
    "LastNLoadPCs",
    "LastNPCs"
};

string state_info_t::to_string()
{
    stringstream ss;
    
    ss  << "PC: " << setw(8) << hex << pc << dec
        << " data_id: " << setw(2) << data_index
        << " vaddr: " << setw(12) << hex << vaddr << dec
        << " vpage: " << setw(12) << hex << vpage << dec
        << " voffset: " << setw(2) << voffset
        << " fa: " << setw(1) << first_access
        << " vclo: " << setw(2) << v_cl_offset
        << " vclwo: " << setw(2) << v_cl_word_offset
        << " vcldwo: " << setw(2) << v_cl_dword_offset
        ;
    
    return ss.str();
}

perceptron_pred_t::perceptron_pred_t(vector<int32_t> _activated_features, vector<int32_t> weight_array_sizes, vector<int32_t> hash_types, float threshold, float max_w, float min_w, float pos_delta, float neg_delta, float pos_thresh, float neg_thresh)
    : activation_threshold(threshold),
      max_weight(max_w),
      min_weight(min_w),
      pos_weight_delta(pos_delta),
      neg_weight_delta(neg_delta),
      pos_train_thresh(pos_thresh),
      neg_train_thresh(neg_thresh)
{
    assert(_activated_features.size() == weight_array_sizes.size());
    assert(_activated_features.size() == hash_types.size());

    num_features = _activated_features.size();
    activated_features = _activated_features;
    for(uint32_t index = 0; index < weight_array_sizes.size(); ++index)
    {
        weights.push_back(weight_array_t(weight_array_sizes[index]));
    }
    feature_hash_types = hash_types;

    cpu = 0;
}

perceptron_pred_t::~perceptron_pred_t()
{

}

void perceptron_pred_t::dump_stats()
{
    cout << "perc_predict_called " << stats.predict.called << endl
         << "perc_predict_pred_true " << stats.predict.pred_true << endl
         << "perc_predict_pred_false " << stats.predict.pred_false << endl
         << "perc_train_called " << stats.train.called << endl
         << "perc_train_incr_weight_match " << stats.train.incr_weight_match << endl
         << "perc_train_incr_weight_mismatch " << stats.train.incr_weight_mismatch << endl
         << "perc_train_decr_weight_match " << stats.train.decr_weight_match << endl
         << "perc_train_decr_weight_mismatch " << stats.train.decr_weight_mismatch << endl
         << endl;

    for(uint32_t feature = 0; feature < num_features; ++feature)
    {
        cout << "perc_feature_" << feature_names[activated_features[feature]] << "_incr_done " << stats.weight.incr_done[activated_features[feature]] << endl
             << "perc_feature_" << feature_names[activated_features[feature]] << "_incr_satu " << stats.weight.incr_satu[activated_features[feature]] << endl
             << "perc_feature_" << feature_names[activated_features[feature]] << "_decr_done " << stats.weight.decr_done[activated_features[feature]] << endl
             << "perc_feature_" << feature_names[activated_features[feature]] << "_decr_satu " << stats.weight.decr_satu[activated_features[feature]] << endl
             << endl;
    }
}

void perceptron_pred_t::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

void perceptron_pred_t::predict(state_info_t *state, bool &prediction, float &perc_weight_sum)
{
    stats.predict.called++;
    vector<uint32_t> weight_indices = generate_indices_from_state(state);
    assert(weight_indices.size() == num_features);

    float cummulative_weight = 0.0;
    for(uint32_t feature = 0; feature < num_features; ++feature)
    {
        assert(weight_indices[feature] < weights[feature].size);
        cummulative_weight += weights[feature].array[weight_indices[feature]]; // sum up all feature weights
    }
    perc_weight_sum = cummulative_weight;

    // simple binary activation
    prediction = (cummulative_weight >= activation_threshold) ? true : false;

    if(prediction)
        stats.predict.pred_true++;
    else
        stats.predict.pred_false++;
}

void perceptron_pred_t::train(state_info_t *state, float perc_weight_sum, bool pred_output, bool true_output)
{
    stats.train.called++;
    MYLOG(warmup_complete[cpu], "==========================");
    MYLOG(warmup_complete[cpu], "perc_train %s perc_weight_sum: %f offchip_real: %d offchip_pred: %d", state->to_string().c_str(), perc_weight_sum, true_output, pred_output);

    vector<uint32_t> weight_indices = generate_indices_from_state(state);
    assert(weight_indices.size() == num_features);

    if(true_output == true)
    {
        if(pred_output == true_output)
        {
            // correctly predicted true
            if(perc_weight_sum >= neg_train_thresh && perc_weight_sum <= pos_train_thresh)
            {
                MYLOG(warmup_complete[cpu], "correct_pred::true increasing weights ...");
                incr_weights(weight_indices);
                stats.train.incr_weight_match++;
            }
            else
            {
                MYLOG(warmup_complete[cpu], "correct_pred::true perc_sum_weight already saturated ...");
            }
        }
        else
        {
            // should have predicted true, but actually predicted false
            MYLOG(warmup_complete[cpu], "incorr_pred::real=true::pred=false increasing weights ...");
            incr_weights(weight_indices);
            stats.train.incr_weight_mismatch++;
        }
    }
    else
    {
        if(pred_output == true_output)
        {
            // correctly predicted false
            if(perc_weight_sum >= neg_train_thresh && perc_weight_sum <= pos_train_thresh)
            {
                MYLOG(warmup_complete[cpu], "correct_pred::false decreasing weights ...");
                decr_weights(weight_indices);
                stats.train.decr_weight_match++;
            }
            else
            {
                MYLOG(warmup_complete[cpu], "correct_pred::false perc_sum_weight already saturated ...");
            }
        }
        else
        {
            // should have predicted false, but actually predicted true
            MYLOG(warmup_complete[cpu], "incorr_pred::real=false::pred=true decreasing weights ...");
            decr_weights(weight_indices);
            stats.train.decr_weight_mismatch++;
        }
    }

    MYLOG(warmup_complete[cpu], "==========================");
}

void perceptron_pred_t::incr_weights(vector<uint32_t> weight_indices)
{
    for(uint32_t feature = 0; feature < num_features; ++feature)
    {
        if((weights[feature].array[weight_indices[feature]] + pos_weight_delta) <= max_weight)
        {
            MYLOG(warmup_complete[cpu], "feature %s index %d old_weight %f", feature_names[activated_features[feature]].c_str(), weight_indices[feature], weights[feature].array[weight_indices[feature]]);
            weights[feature].array[weight_indices[feature]] += pos_weight_delta;
            MYLOG(warmup_complete[cpu], "feature %s index %d new_weight %f", feature_names[activated_features[feature]].c_str(), weight_indices[feature], weights[feature].array[weight_indices[feature]]);
            stats.weight.incr_done[activated_features[feature]]++;
        }
        else
        {
            MYLOG(warmup_complete[cpu], "feature %s index %d saturated_weight %f", feature_names[activated_features[feature]].c_str(), weight_indices[feature], weights[feature].array[weight_indices[feature]]);
            stats.weight.incr_satu[activated_features[feature]]++;
        }
    }
}

void perceptron_pred_t::decr_weights(vector<uint32_t> weight_indices)
{
    for(uint32_t feature = 0; feature < num_features; ++feature)
    {
        if((weights[feature].array[weight_indices[feature]] - neg_weight_delta) >= min_weight)
        {
            MYLOG(warmup_complete[cpu], "feature %s index %d old_weight %f", feature_names[activated_features[feature]].c_str(), weight_indices[feature], weights[feature].array[weight_indices[feature]]);
            weights[feature].array[weight_indices[feature]] -= neg_weight_delta;
            MYLOG(warmup_complete[cpu], "feature %s index %d new_weight %f", feature_names[activated_features[feature]].c_str(), weight_indices[feature], weights[feature].array[weight_indices[feature]]);
            stats.weight.decr_done[activated_features[feature]]++;
        }
        else
        {
            MYLOG(warmup_complete[cpu], "feature %s index %d saturated_weight %f", feature_names[activated_features[feature]].c_str(), weight_indices[feature], weights[feature].array[weight_indices[feature]]);
            stats.weight.decr_satu[activated_features[feature]]++;
        }
    }
}

vector<uint32_t> perceptron_pred_t::generate_indices_from_state(state_info_t *state)
{
    vector<uint32_t> indices;
    for(uint32_t index = 0; index < num_features; ++index)
    {
        indices.push_back(generate_index_from_feature((feature_type_t)activated_features[index], state, 0xdeadbeef, feature_hash_types[index], weights[index].size));
    }
    return indices;
}



}