#include <assert.h>
#include "perc_pred.h"
#include "util.h"

using namespace perc;

uint32_t process_PC(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t folded_pc = folded_xor(state->pc, 2);
    uint32_t hashed_val = HashZoo::getHash((uint32_t)hash_type, folded_pc);
    return (hashed_val % weight_array_size);
}

uint32_t process_Offset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t raw = state->voffset;
    raw = HashZoo::getHash(hash_type, raw);
    return (raw % weight_array_size);
}

uint32_t process_Page(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->vpage;
    uint32_t val = folded_xor(raw, 2);
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_Addr(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->vaddr;
    uint32_t val = folded_xor(raw, 2);
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_FirstAccess(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t raw = state->first_access ? 1 : 0;
    return (raw % weight_array_size);
}

uint32_t process_PC_Offset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->pc;
    uint32_t val = folded_xor(raw, 2);
    val = val << 6;
    val += state->voffset;
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_PC_Page(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->pc;
    raw = raw << 12;
    raw = raw ^ state->vpage;
    uint32_t val = folded_xor(raw, 2);
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_PC_Addr(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->pc;
    raw = raw << 15;
    raw = raw ^ state->vaddr;
    uint32_t val = folded_xor(raw, 2);
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_PC_FirstAccess(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->pc;
    uint32_t val = folded_xor(raw, 2);
    val = val & ((1u << 31) - 1); // zero-out MSB
    if(state->first_access)
        val = val | (1u << 31); // set MSB only if first_access
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_Offset_FirstAccess(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t val = state->voffset;
    val = val & ((1u << 6) - 1);
    if(state->first_access)
        val = val | (1u << 6);
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_CLOffset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t raw = state->v_cl_offset;
    return (raw % weight_array_size);
}

uint32_t process_PC_CLOffset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->pc;
    uint32_t val = folded_xor(raw, 2);
    val = val << 6;
    val += state->v_cl_offset;
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_CLWordOffset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t raw = state->v_cl_word_offset;
    return (raw % weight_array_size);
}

uint32_t process_PC_CLWordOffset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->pc;
    uint32_t val = folded_xor(raw, 2);
    val = val << 4;
    val += state->v_cl_word_offset;
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_CLDWordOffset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t raw = state->v_cl_dword_offset;
    return (raw % weight_array_size);
}

uint32_t process_PC_CLDWordOffset(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint64_t raw = state->pc;
    uint32_t val = folded_xor(raw, 2);
    val = val << 3;
    val += state->v_cl_dword_offset;
    val = HashZoo::getHash(hash_type, val);
    return (val % weight_array_size);
}

uint32_t process_LastNLoadPCs(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t folded_pc = folded_xor(state->last_n_load_pc_sig, 2);
    uint32_t hashed_val = HashZoo::getHash((uint32_t)hash_type, folded_pc);
    return (hashed_val % weight_array_size);
}

uint32_t process_LastNPCs(state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    uint32_t folded_pc = folded_xor(state->last_n_pc_sig, 2);
    uint32_t hashed_val = HashZoo::getHash((uint32_t)hash_type, folded_pc);
    return (hashed_val % weight_array_size);
}

uint32_t perceptron_pred_t::generate_index_from_feature(feature_type_t feature, state_info_t *state, uint64_t metadata, int32_t hash_type, uint32_t weight_array_size)
{
    if(state == NULL) return 0;
    
    switch(feature)
    {
        case feature_type_t::PC:                    return process_PC(state, metadata, hash_type, weight_array_size);
        case feature_type_t::Offset:                return process_Offset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::Page:                  return process_Page(state, metadata, hash_type, weight_array_size);
        case feature_type_t::Addr:                  return process_Addr(state, metadata, hash_type, weight_array_size);
        case feature_type_t::FirstAccess:           return process_FirstAccess(state, metadata, hash_type, weight_array_size);
        case feature_type_t::PC_Offset:             return process_PC_Offset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::PC_Page:               return process_PC_Page(state, metadata, hash_type, weight_array_size);
        case feature_type_t::PC_Addr:               return process_PC_Addr(state, metadata, hash_type, weight_array_size);
        case feature_type_t::PC_FirstAccess:        return process_PC_FirstAccess(state, metadata, hash_type, weight_array_size);
        case feature_type_t::Offset_FirstAccess:    return process_Offset_FirstAccess(state, metadata, hash_type, weight_array_size);
        case feature_type_t::CLOffset:              return process_CLOffset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::PC_CLOffset:           return process_PC_CLOffset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::CLWordOffset:          return process_CLWordOffset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::PC_CLWordOffset:       return process_PC_CLWordOffset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::CLDWordOffset:         return process_CLDWordOffset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::PC_CLDWordOffset:      return process_PC_CLDWordOffset(state, metadata, hash_type, weight_array_size);
        case feature_type_t::LastNLoadPCs:          return process_LastNLoadPCs(state, metadata, hash_type, weight_array_size);
        case feature_type_t::LastNPCs:              return process_LastNPCs(state, metadata, hash_type, weight_array_size);
        default:                                    assert(false);
    }

    return 0;
}
