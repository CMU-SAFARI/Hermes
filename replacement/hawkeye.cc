#include <stdlib.h>
#include <assert.h>
#include "hawkeye.h"
#include "defs.h"

namespace knob
{
    extern uint32_t hawkeye_pred_counter_width;
    extern uint32_t hawkeye_max_rrip;
    extern uint32_t hawkeye_pred_size;
    extern uint32_t hawkeye_pred_hash_type;
    extern uint32_t hawkeye_optgen_hist_len_factor;
}

void HawkeyeRepl::init_knobs()
{

}

void HawkeyeRepl::init_stats()
{
    bzero(&stats, sizeof(stats));   
}

HawkeyeRepl::HawkeyeRepl(string name) : CacheReplBase(name)
{
    init_knobs();
    init_stats();
}

HawkeyeRepl::~HawkeyeRepl()
{

}

void HawkeyeRepl::print_config()
{
    cout << "hawkeye_pred_counter_width " << knob::hawkeye_pred_counter_width << endl
        << "hawkeye_max_rrip " << knob::hawkeye_max_rrip << endl
        << "hawkeye_pred_size " << knob::hawkeye_pred_size << endl
        << "hawkeye_pred_hash_type " << knob::hawkeye_pred_hash_type << endl
        << "hawkeye_optgen_hist_len_factor " << knob::hawkeye_optgen_hist_len_factor << endl
        << endl;
}

void HawkeyeRepl::initialize_replacement()
{
    // init OPTgen
    optgen = new OPTgen();

    // init predictor
    predictor = new HawkeyePred();

    // init RRIP values
    rrip = (uint32_t**)calloc(LLC_SET, sizeof(uint32_t*));
    assert(rrip);
    for(uint32_t set = 0; set < LLC_SET; ++set)
    {
        rrip[set] = (uint32_t*)calloc(LLC_WAY, sizeof(uint32_t));
        assert(rrip[set]);
    }
    // vector<uint32_t> vec;
    // vec.resize(LLC_WAY, 0);
    // rrip.resize(LLC_SET, vec);
    // cout << "Init RRIP dim " << rrip.size() << "x" << rrip[0].size() << endl;

    // init metadata
    metadata = (HawkeyeMetadata***)calloc(LLC_SET, sizeof(HawkeyeMetadata**));
    assert(metadata);
    for(uint32_t set = 0; set < LLC_SET; ++set)
    {
        metadata[set] = (HawkeyeMetadata**)calloc(LLC_WAY, sizeof(HawkeyeMetadata*));
        assert(metadata[set]);
        for(uint32_t way = 0; way < LLC_WAY; ++way)
        {
            metadata[set][way] = new HawkeyeMetadata();
        }
    }
    // vector<HawkeyeMetadata*> vec2;
    // vec2.resize(LLC_WAY, NULL);
    // metadata.resize(LLC_SET, vec2);
    // for(uint32_t set = 0; set < LLC_SET; ++set)
    // {
    //     for(uint32_t way = 0; way < LLC_WAY; ++way)
    //     {
    //         metadata[set][way] = new HawkeyeMetadata();
    //     }
    // }
}

void HawkeyeRepl::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    stats.update_repl_state.called++;

    // call OPTgen to check whether this access would have receieved hit in OPT policy
    bool opt_decision = optgen->update_access(full_addr, set);

    // train the predictor based on decision
    predictor->train(ip, opt_decision);

    // predict whether this access is cache-friendly or cache adverse
    bool prediction = predictor->predict(ip);

    // update RRIP based on the prediction and whether the access is hit or miss
    if(prediction == true) // cache-friendly
    {
        stats.update_repl_state.cache_friendly++;
        if(hit)
        {
            stats.update_repl_state.cache_friendly_hit++;
            rrip[set][way] = 0;
        }
        else
        {
            stats.update_repl_state.cache_friendly_miss++;
            for(uint32_t index = 0; index < LLC_WAY; ++index)
            {
                rrip[set][index]++;
                if(rrip[set][index] > knob::hawkeye_max_rrip) rrip[set][index] = knob::hawkeye_max_rrip;
            }
            rrip[set][way] = 0;
        }
    }
    else // cache-adverse
    {
        stats.update_repl_state.cache_adverse++;
        if(hit)
        {
            stats.update_repl_state.cache_adverse_hit++;
            rrip[set][way] = knob::hawkeye_max_rrip;
        }
        else
        {
            stats.update_repl_state.cache_adverse_miss++;
            rrip[set][way] = knob::hawkeye_max_rrip;
        }
    }
}

uint32_t HawkeyeRepl::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{    
    // cout << "=== Finding victim for set " << set << endl;
    stats.find_victim.called++;
    // find the candidate that has the highest RRIP value
    for(uint32_t way = 0; way < LLC_WAY; ++way)
    {
        // cout << "set " << set << " way " << way << endl;
        if(rrip[set][way] == knob::hawkeye_max_rrip)
        {
            stats.find_victim.max_rrip_found++;
            return way;
        }
    }

    // if no one has max RRIP, then find the maximum RRIP value
    uint32_t max_rrip = 0;
    uint32_t victim = 0;
    for(uint32_t way = 0; way < LLC_WAY; ++way)
    {
        if(rrip[set][way] > max_rrip)
        {
            max_rrip = rrip[set][way];
            victim = way;
        }
    }
    stats.find_victim.max_rrip_not_found++;

    // @RBERA-TODO: we have to detrain the predictor using the PC of the victim

    return victim;
}

void HawkeyeRepl::dump_stats()
{
    optgen->dump_stats();
    predictor->dump_stats();

    cout << "hawkeye.repl.update_repl_state.called " << stats.update_repl_state.called << endl
        << "hawkeye.repl.update_repl_state.cache_friendly " << stats.update_repl_state.cache_friendly << endl
        << "hawkeye.repl.update_repl_state.cache_friendly_hit " << stats.update_repl_state.cache_friendly_hit << endl
        << "hawkeye.repl.update_repl_state.cache_friendly_miss " << stats.update_repl_state.cache_friendly_miss << endl
        << "hawkeye.repl.update_repl_state.cache_adverse " << stats.update_repl_state.cache_adverse << endl
        << "hawkeye.repl.update_repl_state.cache_adverse_hit " << stats.update_repl_state.cache_adverse_hit << endl
        << "hawkeye.repl.update_repl_state.cache_adverse_miss " << stats.update_repl_state.cache_adverse_miss << endl
        << endl
        << "hawkeye.repl.find_victim.called " << stats.find_victim.called << endl
        << "hawkeye.repl.find_victim.max_rrip_found " << stats.find_victim.max_rrip_found << endl
        << "hawkeye.repl.find_victim.max_rrip_not_found " << stats.find_victim.max_rrip_not_found << endl
        << endl;
}

/* Hawkeye Predictor */
HawkeyePred::HawkeyePred()
{
    confidence = (Counter**)calloc(knob::hawkeye_pred_size, sizeof(Counter*));
    assert(confidence);
    for(uint32_t index = 0; index < knob::hawkeye_pred_size; ++index)
    {
        confidence[index] = new Counter(knob::hawkeye_pred_counter_width);
    }

    bzero(&stats, sizeof(stats));
}

HawkeyePred::~HawkeyePred()
{

}

/* opt_decision = true means OPT had generated a hit */
void HawkeyePred::train(uint64_t ip, bool opt_decision)
{
    stats.train.called++;
    uint32_t index = gen_index(ip);
    if(opt_decision)
    {
        stats.train.incr++;
        confidence[index]->incr();
    }
    else
    {
        stats.train.decr++;
        confidence[index]->decr();
    }
}

/* A true prediction means OPT would have predicted hit */
bool HawkeyePred::predict(uint64_t ip)
{
    stats.predict.called++;
    uint32_t index = gen_index(ip);
    if(confidence[index]->val() > confidence[index]->max()/2) // predict cache-friendly
    {
        stats.predict.cache_friendly++;
        return true;
    }
    else
    {
        stats.predict.cache_adverse++;
        return false;
    }
}

uint32_t HawkeyePred::gen_index(uint64_t ip)
{
    return (HashZoo::getHash(knob::hawkeye_pred_hash_type, folded_xor(ip, 2)) % knob::hawkeye_pred_size);
}

void HawkeyePred::dump_stats()
{
    cout << "hawkeye.predictor.train.called " << stats.train.called << endl
        << "hawkeye.predictor.train.incr " << stats.train.incr << endl
        << "hawkeye.predictor.train.decr " << stats.train.decr << endl
        << endl
        << "hawkeye.predictor.predict.called " << stats.predict.called << endl
        << "hawkeye.predictor.predict.cache_friendly " << stats.predict.cache_friendly << endl
        << "hawkeye.predictor.predict.cache_adverse " << stats.predict.cache_adverse << endl
        << endl;
}

/* OPTgen */
OPTgen::OPTgen()
{
    history_len = LLC_WAY * knob::hawkeye_optgen_hist_len_factor;
    timestamp.resize(LLC_SET, 0);

    // init access_history
    deque<pair<uint64_t, uint64_t> > d;
    d.clear();
    access_history.resize(LLC_SET, d);

    // init occupancy_vector
    deque<uint32_t> d2;
    d2.clear();
    occupancy_vector.resize(LLC_SET, d2);

    bzero(&stats, sizeof(stats));
}

OPTgen::~OPTgen()
{

}

/* Returns true if OPT would have produced a hit */
bool OPTgen::update_access(uint64_t address, uint32_t set)
{
    stats.update.called++;
    bool opt_decision = false;
    uint64_t ca_addr = (address >> LOG2_BLOCK_SIZE) << LOG2_BLOCK_SIZE;

    // add the new access to access_history
    if(access_history[set].size() == history_len)
    {
        stats.update.access_history_spill++;
        access_history[set].pop_front();
    }
    access_history[set].push_back(pair<uint64_t, uint64_t>(ca_addr, timestamp[set]));

    // add new element to the occupancy vector
    if(occupancy_vector[set].size() == history_len)
    {
        stats.update.occupancy_vector_spill++;
        occupancy_vector[set].pop_front();
    }
    occupancy_vector[set].push_back(0);

    // at this point, length of occupancy_vector and access_history should be same
    assert(access_history[set].size() == occupancy_vector[set].size());

    // first check whether this is the first access or not
    int32_t prev_occurence = -1;
    for(int32_t index = access_history[set].size()-2; index >= 0; --index)
    {
        if(access_history[set].back().first == access_history[set][index].first)
        {
            prev_occurence = index;
            break;
        }
    }

    if(prev_occurence == -1) // this is the first access
    {
        opt_decision = false;
        stats.update.first_access++;
    }
    else
    {
        stats.update.reused_access++;
        assert(prev_occurence >= 0);
        bool opt_miss = false;
        for(int32_t index = occupancy_vector[set].size()-2; index >= prev_occurence; --index)
        {
            assert(occupancy_vector[set][index] <= LLC_WAY);
            if(occupancy_vector[set][index] == LLC_WAY)
            {
                opt_miss = true; // OPT would have produced a miss
                break;
            }
        }

        // update occupancy_vector only if OPT would have produced hit
        if(!opt_miss)
        {
            for(int32_t index = occupancy_vector[set].size()-2; index >= prev_occurence; --index)
            {
                occupancy_vector[set][index]++;
            }
            opt_decision = true;
            stats.update.opt_hit++;
        }
        else
        {
            opt_decision = false;
            stats.update.opt_miss++;
        }
    }

    timestamp[set]++;
    return opt_decision;
}

void OPTgen::dump_stats()
{
    cout << "hawkeye.OPTgen.update.called " << stats.update.called << endl
        << "hawkeye.OPTgen.update.access_history_spill " << stats.update.access_history_spill << endl
        << "hawkeye.OPTgen.update.occupancy_vector_spill " << stats.update.occupancy_vector_spill << endl
        << "hawkeye.OPTgen.update.first_access " << stats.update.first_access << endl
        << "hawkeye.OPTgen.update.reused_access " << stats.update.reused_access << endl
        << "hawkeye.OPTgen.update.opt_hit " << stats.update.opt_hit << endl
        << "hawkeye.OPTgen.update.opt_miss " << stats.update.opt_miss << endl
        << endl;
}
