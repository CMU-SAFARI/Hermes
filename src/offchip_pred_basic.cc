#include <iostream>
#include <algorithm>
#include "string.h"
#include "util.h"
#include "offchip_pred_basic.h"
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
#	define MYLOG(cond, ...) {}
#endif

namespace knob
{
    extern uint32_t ocp_basic_table_size;
    extern uint32_t ocp_basic_counter_width;
    extern float    ocp_basic_conf_thresh;
    extern uint32_t ocp_basic_hash_type;
    extern uint32_t ocp_basic_include_data_index_type;
    extern uint32_t ocp_basic_pc_buf_size;
    extern uint32_t ocp_basic_feature_type;
    extern uint32_t ocp_basic_count_modulo;
    extern uint32_t ocp_basic_page_buf_size;
}

const char* map_ocp_basic_feature_type_string[] = {"PC", "PC_count", "PC_data_index", "PC_offset", "PC_Page", "PC_Addr", "CL_first_access"};

string ocp_basic_get_feature_string(uint32_t feature)
{
    assert(feature < num_ocp_basic_feature_types);
    return map_ocp_basic_feature_type_string[feature];
}

void OffchipPredBasic::print_config()
{
    cout << "ocp_basic_table_size " << knob::ocp_basic_table_size << endl
         << "ocp_basic_counter_width " << knob::ocp_basic_counter_width << endl
         << "ocp_basic_conf_thresh " << knob::ocp_basic_conf_thresh << endl
         << "ocp_basic_hash_type " << knob::ocp_basic_hash_type << endl
         << "ocp_basic_include_data_index_type " << knob::ocp_basic_include_data_index_type << endl
         << "ocp_basic_pc_buf_size " << knob::ocp_basic_pc_buf_size << endl
         << "ocp_basic_feature_type " << ocp_basic_get_feature_string(knob::ocp_basic_feature_type) << endl
         << "ocp_basic_count_modulo " << knob::ocp_basic_count_modulo << endl
         << "ocp_basic_page_buf_size " << knob::ocp_basic_page_buf_size << endl
         << endl;
}

void OffchipPredBasic::dump_stats()
{
    cout << "ocp_basic_train_called " << stats.train.called << endl
         << "ocp_basic_train_went_offchip " << stats.train.went_offchip << endl
         << "ocp_basic_predict_called " << stats.predict.called << endl
         << "ocp_basic_predict_offchip_miss " << stats.predict.offchip_miss << endl
         << "ocp_basic_predict_offchip_hit " << stats.predict.offchip_hit << endl
         << "ocp_basic_pc_buf_lookup_called " << stats.pc_buf.called << endl
         << "ocp_basic_pc_buf_lookup_hit " << stats.pc_buf.hit << endl
         << "ocp_basic_pc_buf_lookup_eviction " << stats.pc_buf.eviction << endl
         << "ocp_basic_pc_buf_lookup_insertion " << stats.pc_buf.insertion << endl
         << "ocp_basic_page_buf_lookup_called " << stats.page_buf.called << endl
         << "ocp_basic_page_buf_lookup_hit " << stats.page_buf.hit << endl
         << "ocp_basic_page_buf_lookup_eviction " << stats.page_buf.eviction << endl
         << "ocp_basic_page_buf_lookup_insertion " << stats.page_buf.insertion << endl
         << endl
         << "ocp_unique_pcs " << unique_pcs.size() << endl
         << "ocp_unique_pages " << unique_pages.size() << endl
         << endl;
}

void OffchipPredBasic::reset_stats()
{
    bzero(&stats, sizeof(stats));
}

OffchipPredBasic::OffchipPredBasic(uint32_t _cpu, string _type, uint64_t _seed) : OffchipPredBase(_cpu, _type, _seed)
{
    bzero(&stats, sizeof(stats));

    for(uint32_t index = 0; index < knob::ocp_basic_table_size; ++index)
    { 
        ocp_basic_entry_t entry;
        entry.total.init(knob::ocp_basic_counter_width);
        entry.miss.init(knob::ocp_basic_counter_width);
        m_table.push_back(entry);
    }

}

OffchipPredBasic::~OffchipPredBasic()
{

}

bool OffchipPredBasic::predict(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    uint64_t load_pc = arch_instr->ip;
    uint64_t vaddr = lq_entry->virtual_address;
    uint64_t vpage = vaddr >> LOG2_PAGE_SIZE;
    uint32_t voffset = (vaddr >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    // get control-flow related features
    uint32_t count = 0;
    lookup_pc(load_pc, count);

    // get data-flow related features
    bool first_access = false;
    lookup_address(vaddr, vpage, voffset, first_access);

    // save all features used for this prediction in LQ entry
    // this would be required again while training the predictor
    assert(lq_entry->ocp_feature == NULL);
    ocp_basic_feature_t *feature = new ocp_basic_feature_t();
    feature->pc_count = count;
    feature->first_access = first_access;
    lq_entry->ocp_feature = feature;

    // compute index
    uint32_t index = get_hash(load_pc, count, data_index, vaddr, vpage, voffset, first_access);
    assert(index < knob::ocp_basic_table_size);

    stats.predict.called++;
    if (m_table[index].total.val() > 0)
    {
        float ratio = (float)m_table[index].miss.val() / m_table[index].total.val();
        if (ratio >= knob::ocp_basic_conf_thresh)
        {
            stats.predict.offchip_miss++;
            MYLOG(warmup_complete[cpu], "load_pc: %x count: %d data_idx: %d ratio: %f predicted: MISS", load_pc, count, data_index, ratio);
            return true;
        }
        else
        {
            stats.predict.offchip_hit++;
            MYLOG(warmup_complete[cpu], "load_pc: %x count: %d data_idx: %d ratio: %f predicted: HIT", load_pc, count, data_index, ratio);
            return false;
        }
    }
    else
    {
        stats.predict.offchip_miss++;
        return false;
    }
}

void OffchipPredBasic::train(ooo_model_instr *arch_instr, uint32_t data_index, LSQ_ENTRY *lq_entry)
{
    uint64_t load_pc = arch_instr->ip;
    uint64_t vaddr = lq_entry->virtual_address;
    uint64_t vpage = vaddr >> LOG2_PAGE_SIZE;
    uint32_t voffset = (vaddr >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    // retreive all features from LQ entry that were used for the prediction
    assert(lq_entry->ocp_feature);
    ocp_basic_feature_t *feature = (ocp_basic_feature_t*) lq_entry->ocp_feature;
    uint32_t count = feature->pc_count;
    bool first_access = feature->first_access;

    uint32_t index = get_hash(load_pc, count, data_index, vaddr, vpage, voffset, first_access);
    assert(index < knob::ocp_basic_table_size);
    
    // simple table-based training
    stats.train.called++;
    m_table[index].total.incr();
    if(lq_entry->went_offchip)
    {
        stats.train.went_offchip++;
        m_table[index].miss.incr();
    }

    MYLOG(warmup_complete[cpu], 
          "load_pc: %x count: %d data_idx: %d vaddr: %x vpage: %x voffset: %d offchip_real: %d offchip_pred: %d table_index: %d total_counter: %d miss_counter: %d", 
          load_pc, count, data_index, vaddr, vpage, vofffset, (uint32_t)lq_entry->went_offchip, (uint32_t)lq_entry->went_offchip_pred, index, m_table[index].total.val(), m_table[index].miss.val());
}

uint32_t OffchipPredBasic::get_hash(uint64_t load_pc, uint32_t count, uint32_t data_index, uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool first_access)
{
    switch(knob::ocp_basic_feature_type)
    {
        case ocp_basic_pc:              return get_hash_pc(load_pc);
        case ocp_basic_pc_count:        return get_hash_pc_count(load_pc, count);
        case ocp_basic_pc_data_index:   return get_hash_pc_data_index(load_pc, data_index);
        case ocp_basic_pc_offset:       return get_hash_pc_offset(load_pc, voffset);
        case ocp_basic_pc_page:         return get_hash_pc_page(load_pc, vpage);
        case ocp_basic_pc_addr:         return get_hash_pc_addr(load_pc, vaddr);
        case ocp_basic_cl_first_access: return get_hash_cl_first_access(first_access);
        default:                        assert(false);
    }
}

uint32_t OffchipPredBasic::get_hash_pc(uint64_t load_pc)
{
    uint32_t folded_pc = folded_xor(load_pc, 2);
    uint32_t hashed_val = HashZoo::getHash(knob::ocp_basic_hash_type, folded_pc);
    
    return (hashed_val % knob::ocp_basic_table_size);
}

uint32_t OffchipPredBasic::get_hash_pc_count(uint64_t load_pc, uint32_t count)
{
    uint64_t raw_data = load_pc;
    uint32_t count_processed = count % knob::ocp_basic_count_modulo;
    raw_data = (raw_data << 4) + count_processed;
    return (HashZoo::getHash(knob::ocp_basic_hash_type, folded_xor(raw_data, 2)) % knob::ocp_basic_table_size);
}

uint32_t OffchipPredBasic::get_hash_pc_data_index(uint64_t load_pc, uint32_t data_index)
{
    uint32_t folded_pc = folded_xor(load_pc, 2);
    uint32_t hashed_val = 0;
    
    switch(knob::ocp_basic_include_data_index_type)
    {
        case 0:
            folded_pc = (folded_pc << 2) + data_index;
            hashed_val = HashZoo::getHash(knob::ocp_basic_hash_type, folded_pc);
            break;
        case 1:
            hashed_val = HashZoo::getHash(knob::ocp_basic_hash_type, folded_pc);
            hashed_val = (hashed_val << 2) + data_index;
            break;
        default:
            assert(false);
    }

    return (hashed_val % knob::ocp_basic_table_size);
}

uint32_t OffchipPredBasic::get_hash_pc_offset(uint64_t load_pc, uint32_t voffset)
{
    uint64_t raw_data = load_pc;
    raw_data = raw_data << 6;
    raw_data += voffset;
    return (HashZoo::getHash(knob::ocp_basic_hash_type, folded_xor(raw_data, 2)) % knob::ocp_basic_table_size);
}

uint32_t OffchipPredBasic::get_hash_pc_page(uint64_t load_pc, uint64_t vpage)
{
    uint64_t raw_data = load_pc;
    raw_data = raw_data << 12;
    raw_data ^= vpage;
    return (HashZoo::getHash(knob::ocp_basic_hash_type, folded_xor(raw_data, 2)) % knob::ocp_basic_table_size);
}

uint32_t OffchipPredBasic::get_hash_pc_addr(uint64_t load_pc, uint64_t vaddr)
{
    uint64_t raw_data = load_pc;
    raw_data = raw_data << 12;
    raw_data ^= vaddr;
    return (HashZoo::getHash(knob::ocp_basic_hash_type, folded_xor(raw_data, 2)) % knob::ocp_basic_table_size);
}

uint32_t OffchipPredBasic::get_hash_cl_first_access(bool first_access)
{
    uint32_t raw_data =  first_access ? 1 : 0;
    return (raw_data % knob::ocp_basic_table_size);
}


void OffchipPredBasic::lookup_pc(uint64_t load_pc, uint32_t &count)
{
    stats.pc_buf.called++;
    unique_pcs.insert(load_pc);

    ocp_basic_pc_buf_entry_t entry;
    auto it = find_if(m_pc_buffer.begin(), m_pc_buffer.end(), [load_pc](const ocp_basic_pc_buf_entry_t& entry){return entry.pc == load_pc;});
    
    if(it != m_pc_buffer.end()) // pc hit
    {
        stats.pc_buf.hit++;
        entry.pc = load_pc;
        entry.count = it->count + 1;
        count = entry.count;
        m_pc_buffer.erase(it);
        m_pc_buffer.push_back(entry);
    }
    else // pc miss
    {
        if(m_pc_buffer.size() >= knob::ocp_basic_pc_buf_size) // eviction
        {
            m_pc_buffer.pop_front();
            stats.pc_buf.eviction++;
        }

        entry.pc = load_pc;
        entry.count = 1;
        count = entry.count;
        m_pc_buffer.push_back(entry);
        stats.pc_buf.insertion++;
    }
}

void OffchipPredBasic::lookup_address(uint64_t vaddr, uint64_t vpage, uint32_t voffset, bool &first_access)
{
    stats.page_buf.called++;
    unique_pages.insert(vpage);

    ocp_basic_page_buf_entry_t entry;
    auto it = find_if(m_page_buffer.begin(), m_page_buffer.end(), [vpage](ocp_basic_page_buf_entry_t& entry) {return entry.page == vpage;});
    
    if (it != m_page_buffer.end()) // page hit
    {
        first_access = !it->bmp_access.test(voffset);
        it->bmp_access.set(voffset);
        it->age = 0;
        entry = (*it);
        m_page_buffer.erase(it);
        m_page_buffer.push_back(entry);
        stats.page_buf.hit++;
    }
    else
    {
        if (m_page_buffer.size() >= knob::ocp_basic_page_buf_size)
        {
            m_page_buffer.pop_front();
            stats.page_buf.eviction++;
        }

        entry.page = vpage;
        entry.bmp_access.set(voffset);
        entry.age = 0;
        m_page_buffer.push_back(entry);
        first_access = true;
        stats.page_buf.insertion++;
    }
}