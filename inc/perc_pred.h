#ifndef PERCEPTRON_PREDICTOR_H
#define PERCEPTRON_PREDICTOR_H

#include <vector>
#include <stdint.h>
#include <string>
using namespace std;

namespace perc
{

typedef enum
{
    PC = 0,                     // 0
    Offset,                     // 1
    Page,                       // 2
    Addr,                       // 3
    FirstAccess,                // 4
    PC_Offset,                  // 5
    PC_Page,                    // 6
    PC_Addr,                    // 7
    PC_FirstAccess,             // 8
    Offset_FirstAccess,         // 9
    CLOffset,                   // 10
    PC_CLOffset,                // 11
    CLWordOffset,               // 12
    PC_CLWordOffset,            // 13
    CLDWordOffset,              // 14
    PC_CLDWordOffset,           // 15
    LastNLoadPCs,               // 16
    LastNPCs,                   // 17

    num_feature_types
} feature_type_t;

extern string feature_names[num_feature_types];

struct state_info_t
{
    uint64_t pc;
    uint64_t last_n_load_pc_sig;
    uint64_t last_n_pc_sig;
    uint32_t data_index;
    uint64_t vaddr;
    uint64_t vpage;
    uint64_t voffset;
    bool first_access;
    uint32_t v_cl_offset;
    uint32_t v_cl_word_offset;
    uint32_t v_cl_dword_offset;

    state_info_t()
    {
        pc = 0;
        last_n_load_pc_sig = 0;
        last_n_pc_sig = 0;
        data_index = 0;
        vaddr = 0;
        vpage = 0;
        voffset = 0;
        first_access = false;
        v_cl_offset = 0;
        v_cl_word_offset = 0;
        v_cl_dword_offset = 0;
    }

    string to_string();
};

class weight_array_t
{
    public:
        uint32_t size;
        vector<float> array;

    public:
        weight_array_t(uint32_t _size)
        {
            size = _size;
            array.resize(size, 0.0);
        }
        ~weight_array_t(){}
};

class perceptron_pred_t
{
    private:
        float activation_threshold;
        float max_weight;
        float min_weight;
        float pos_weight_delta;
        float neg_weight_delta;
        float pos_train_thresh;
        float neg_train_thresh;
        uint32_t num_features;
        vector<int32_t> activated_features;
        vector<weight_array_t> weights;
        vector<int32_t> feature_hash_types;

        // optional
        int cpu;

        struct
        {
            struct
            {
                uint64_t called;
                uint64_t pred_true;
                uint64_t pred_false;
            } predict;

            struct
            {
                uint64_t called;
                uint64_t incr_weight_match;
                uint64_t incr_weight_mismatch;
                uint64_t decr_weight_match;
                uint64_t decr_weight_mismatch;
            } train;

            struct
            {
                uint64_t incr_done[feature_type_t::num_feature_types];
                uint64_t incr_satu[feature_type_t::num_feature_types];
                uint64_t decr_done[feature_type_t::num_feature_types];
                uint64_t decr_satu[feature_type_t::num_feature_types];
            } weight;
            
        } stats;

        // auxiliary functions
        void incr_weights(vector<uint32_t> weight_indices);
        void decr_weights(vector<uint32_t> weight_indices);
        vector<uint32_t> generate_indices_from_state(state_info_t *state);

        // interface function
        uint32_t generate_index_from_feature(feature_type_t feature, state_info_t *info, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size);

    public:
        perceptron_pred_t(vector<int32_t> _activated_features, vector<int32_t> weight_array_sizes, vector<int32_t> feature_hash_types, float threshold, float max_w, float min_w, float pos_delta, float neg_delta, float pos_thresh, float neg_thresh);
        ~perceptron_pred_t();

        void set_cpu(int _cpu) {cpu = _cpu;}
        int get_cpu(){return cpu;}

        void predict(state_info_t *state, bool &prediction, float &perc_weight_sum);
        void train(state_info_t *state, float perc_weight_sum, bool pred_output, bool true_output);

        inline void set_activation_threshold(float act_thresh) {activation_threshold = act_thresh;}
        inline float get_activation_threshold() {return activation_threshold;}

        void dump_stats();
        void reset_stats();
};


}


#endif /* PERCEPTRON_PREDICTOR_H */

