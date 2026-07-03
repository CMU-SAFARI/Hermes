#include <assert.h>
#include "perc_pred.h"
#include "util.h"

using namespace perc;

uint32_t process_PC(state_info_t *state, uint64_t metadata, int32_t hash_type,
                    uint32_t weight_array_size)
{
  uint32_t folded_pc  = folded_xor(state->pc, 2);
  uint32_t hashed_val = HashZoo::getHash((uint32_t)hash_type, folded_pc);
  return (hashed_val % weight_array_size);
}

uint32_t process_Offset(state_info_t *state, uint64_t metadata,
                        int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t raw = state->offset;
  raw          = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t process_Page(state_info_t *state, uint64_t metadata, int32_t hash_type,
                      uint32_t weight_array_size)
{
  uint64_t raw = state->page;
  uint32_t val = folded_xor(raw, 2);
  val          = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_Addr(state_info_t *state, uint64_t metadata, int32_t hash_type,
                      uint32_t weight_array_size)
{
  uint64_t raw = state->addr;
  uint32_t val = folded_xor(raw, 2);
  val          = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_FirstAccess(state_info_t *state, uint64_t metadata,
                             int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t raw = state->first_access ? 1 : 0;
  return (raw % weight_array_size);
}

uint32_t process_PC_Offset(state_info_t *state, uint64_t metadata,
                           int32_t hash_type, uint32_t weight_array_size)
{
  uint64_t raw = state->pc;
  uint32_t val = folded_xor(raw, 2);
  val          = val << 6;
  val += state->offset;
  val = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_PC_Page(state_info_t *state, uint64_t metadata,
                         int32_t hash_type, uint32_t weight_array_size)
{
  uint64_t raw = state->pc;
  raw          = raw << 12;
  raw          = raw ^ state->page;
  uint32_t val = folded_xor(raw, 2);
  val          = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_PC_Addr(state_info_t *state, uint64_t metadata,
                         int32_t hash_type, uint32_t weight_array_size)
{
  uint64_t raw = state->pc;
  raw          = raw << 15;
  raw          = raw ^ state->addr;
  uint32_t val = folded_xor(raw, 2);
  val          = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_PC_FirstAccess(state_info_t *state, uint64_t metadata,
                                int32_t hash_type, uint32_t weight_array_size)
{
  uint64_t raw = state->pc;
  uint32_t val = folded_xor(raw, 2);
  val          = val & ((1u << 31) - 1);  // zero-out MSB
  if (state->first_access) {
    val = val | (1u << 31);  // set MSB only if first_access
  }
  val = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_Offset_FirstAccess(state_info_t *state, uint64_t metadata,
                                    int32_t  hash_type,
                                    uint32_t weight_array_size)
{
  uint32_t val = state->offset;
  val          = val & ((1u << 6) - 1);
  if (state->first_access) {
    val = val | (1u << 6);
  }
  val = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_CLOffset(state_info_t *state, uint64_t metadata,
                          int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t raw = state->cl_offset;
  return (raw % weight_array_size);
}

uint32_t process_PC_CLOffset(state_info_t *state, uint64_t metadata,
                             int32_t hash_type, uint32_t weight_array_size)
{
  uint64_t raw = state->pc;
  uint32_t val = folded_xor(raw, 2);
  val          = val << 6;
  val += state->cl_offset;
  val = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_CLWordOffset(state_info_t *state, uint64_t metadata,
                              int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t raw = state->cl_word_offset;
  return (raw % weight_array_size);
}

uint32_t process_PC_CLWordOffset(state_info_t *state, uint64_t metadata,
                                 int32_t hash_type, uint32_t weight_array_size)
{
  uint64_t raw = state->pc;
  uint32_t val = folded_xor(raw, 2);
  val          = val << 4;
  val += state->cl_word_offset;
  val = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_CLDWordOffset(state_info_t *state, uint64_t metadata,
                               int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t raw = state->cl_dword_offset;
  return (raw % weight_array_size);
}

uint32_t process_PC_CLDWordOffset(state_info_t *state, uint64_t metadata,
                                  int32_t hash_type, uint32_t weight_array_size)
{
  uint64_t raw = state->pc;
  uint32_t val = folded_xor(raw, 2);
  val          = val << 3;
  val += state->cl_dword_offset;
  val = HashZoo::getHash(hash_type, val);
  return (val % weight_array_size);
}

uint32_t process_LastNLoadPCs(state_info_t *state, uint64_t metadata,
                              int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t folded_pc  = folded_xor(state->last_n_load_pc_sig, 2);
  uint32_t hashed_val = HashZoo::getHash((uint32_t)hash_type, folded_pc);
  return (hashed_val % weight_array_size);
}

uint32_t process_LastNPCs(state_info_t *state, uint64_t metadata,
                          int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t folded_pc  = folded_xor(state->last_n_pc_sig, 2);
  uint32_t hashed_val = HashZoo::getHash((uint32_t)hash_type, folded_pc);
  return (hashed_val % weight_array_size);
}

uint32_t process_PageReuseCount(state_info_t *state, uint64_t metadata,
                                int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t raw = state->page_reuse_count;
  raw          = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t process_PageOffchipCount(state_info_t *state, uint64_t metadata,
                                  int32_t hash_type, uint32_t weight_array_size)
{
  uint32_t raw = state->page_offchip_count;
  raw          = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t process_PageSpatialFootprint(state_info_t *state, uint64_t metadata,
                                      int32_t  hash_type,
                                      uint32_t weight_array_size)
{
  // the full 64-bit line-access bitmap of the page, mixed down non-linearly
  // (fmix64) so distinct spatial patterns land in distinct buckets
  uint32_t raw = fmix64(state->page_spatial_footprint);
  raw          = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t process_LastNDeltas(state_info_t *state, uint64_t metadata,
                             int32_t hash_type, uint32_t weight_array_size)
{
  // last 4 intra-page deltas, 7-bit signed each, packed into 28 bits
  uint32_t raw = state->last_n_deltas_sig;
  raw          = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t process_PageOffsetRegion(state_info_t *state, uint64_t metadata,
                                  int32_t hash_type, uint32_t weight_array_size)
{
  // coarse bucket of the in-page line offset (which 2^log2-line slice of the
  // page the request falls in)
  uint32_t raw = state->page_offset_region;
  raw          = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t process_RegionID(state_info_t *state, uint64_t metadata,
                          int32_t hash_type, uint32_t weight_array_size)
{
  // coarse memory-region bucket (addr >> region_size_log2): does this whole
  // region tend to go off-chip? Mixed down non-linearly (fmix64).
  uint32_t raw = fmix64(state->region_id);
  raw          = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t process_PageMissRatio(state_info_t *state, uint64_t metadata,
                               int32_t hash_type, uint32_t weight_array_size)
{
  // division-free miss ratio: the (offchip, trained) outcome pair as one
  // index, both counts saturated to 5 bits. Each (numerator, denominator)
  // cell learns its own weight, so the model itself learns how much to
  // trust small-sample pages.
  uint32_t offchip = state->page_offchip_count;
  uint32_t trained = state->page_trained_count;
  offchip          = (offchip > 31) ? 31 : offchip;
  trained          = (trained > 31) ? 31 : trained;
  uint32_t raw     = (offchip << 5) | trained;
  raw              = HashZoo::getHash(hash_type, raw);
  return (raw % weight_array_size);
}

uint32_t perceptron_pred_t::generate_index_from_feature(
    feature_type_t feature, state_info_t *state, uint64_t metadata,
    int32_t hash_type, uint32_t weight_array_size)
{
  if (state == NULL) {
    return 0;
  }

  switch (feature) {
  case feature_type_t::PC:
    return process_PC(state, metadata, hash_type, weight_array_size);
  case feature_type_t::Offset:
    return process_Offset(state, metadata, hash_type, weight_array_size);
  case feature_type_t::Page:
    return process_Page(state, metadata, hash_type, weight_array_size);
  case feature_type_t::Addr:
    return process_Addr(state, metadata, hash_type, weight_array_size);
  case feature_type_t::FirstAccess:
    return process_FirstAccess(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PC_Offset:
    return process_PC_Offset(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PC_Page:
    return process_PC_Page(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PC_Addr:
    return process_PC_Addr(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PC_FirstAccess:
    return process_PC_FirstAccess(state, metadata, hash_type,
                                  weight_array_size);
  case feature_type_t::Offset_FirstAccess:
    return process_Offset_FirstAccess(state, metadata, hash_type,
                                      weight_array_size);
  case feature_type_t::CLOffset:
    return process_CLOffset(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PC_CLOffset:
    return process_PC_CLOffset(state, metadata, hash_type, weight_array_size);
  case feature_type_t::CLWordOffset:
    return process_CLWordOffset(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PC_CLWordOffset:
    return process_PC_CLWordOffset(state, metadata, hash_type,
                                   weight_array_size);
  case feature_type_t::CLDWordOffset:
    return process_CLDWordOffset(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PC_CLDWordOffset:
    return process_PC_CLDWordOffset(state, metadata, hash_type,
                                    weight_array_size);
  case feature_type_t::LastNLoadPCs:
    return process_LastNLoadPCs(state, metadata, hash_type, weight_array_size);
  case feature_type_t::LastNPCs:
    return process_LastNPCs(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PageReuseCount:
    return process_PageReuseCount(state, metadata, hash_type,
                                  weight_array_size);
  case feature_type_t::PageOffchipCount:
    return process_PageOffchipCount(state, metadata, hash_type,
                                    weight_array_size);
  case feature_type_t::PageSpatialFootprint:
    return process_PageSpatialFootprint(state, metadata, hash_type,
                                        weight_array_size);
  case feature_type_t::PageMissRatio:
    return process_PageMissRatio(state, metadata, hash_type, weight_array_size);
  case feature_type_t::LastNDeltas:
    return process_LastNDeltas(state, metadata, hash_type, weight_array_size);
  case feature_type_t::RegionID:
    return process_RegionID(state, metadata, hash_type, weight_array_size);
  case feature_type_t::PageOffsetRegion:
    return process_PageOffsetRegion(state, metadata, hash_type,
                                    weight_array_size);
  default:
    assert(false);
  }

  return 0;
}
