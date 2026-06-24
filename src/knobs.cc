#include <iostream>
#include <string>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "knobs.h"
#include "ini.h"
#include "defs.h"
using namespace std;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

namespace knob
{
// All scalar and int/float-array knobs are defined from knobs.def.
#define DEF_KNOB(opt, name, type, parser, defval) type name = defval;
#include "knobs.def"
#undef DEF_KNOB

// The only hand-written knobs left: the prefetcher type lists, which accumulate
// across repeated --*_prefetcher_types options instead of a plain assignment.
// (Derived knobs are in knobs.def with don't-care defaults; their real values
// are computed in compute_derived_knobs() after parsing.)
vector<string> l1d_prefetcher_types;
vector<string> l2c_prefetcher_types;
vector<string> llc_prefetcher_types;
}  // namespace knob

// --------------------------------------------------------------------------
// Parser helpers.
// --------------------------------------------------------------------------

int32_t get_int32(const char *str)
{
  return atoi(str);
}

uint32_t get_uint32(const char *str)
{
  return atoi(str);
}

uint64_t get_uint64(const char *str)
{
  return atol(str);
}

bool get_bool(const char *str)
{
  return !strcmp(str, "true") ? true : false;
}

string get_string(const char *str)
{
  return string(str);
}

float get_float(const char *str)
{
  return atof(str);
}

double get_double(const char *str)
{
  return strtod(str, NULL);
}

// base-0 helpers: accept decimal or 0x-hex (used for addresses / bit masks).
uint32_t get_hex32(const char *str)
{
  return (uint32_t)strtoul(str, NULL, 0);
}

uint64_t get_hex64(const char *str)
{
  return (uint64_t)strtoull(str, NULL, 0);
}

vector<int32_t> get_int32v(const char *str)
{
  std::vector<int32_t> value;
  char                *tmp_str = strdup(str);
  char                *pch     = strtok(tmp_str, ",");
  while (pch) {
    value.push_back(strtol(pch, NULL, 0));
    pch = strtok(NULL, ",");
  }
  free(tmp_str);
  return value;
}

vector<float> get_floatv(const char *str)
{
  std::vector<float> value;
  char              *tmp_str = strdup(str);
  char              *pch     = strtok(tmp_str, ",");
  while (pch) {
    value.push_back(atof(pch));
    pch = strtok(NULL, ",");
  }
  free(tmp_str);
  return value;
}

// --------------------------------------------------------------------------
// Parsing.
// --------------------------------------------------------------------------

// Apply one parsed (name, value) to its knob. Scalar and int/float-array knobs
// come straight from knobs.def; only the accumulating prefetcher lists are
// hand-written. Asserts and derived knobs are handled after all parsing, in
// validate_knobs() and compute_derived_knobs().
int apply_knob(void *user, const char *section, const char *name,
               const char *value)
{
  char config_file_name[MAX_LEN];

  if (MATCH("", "config")) {
    strcpy(config_file_name, value);
    parse_config(config_file_name);
  }
  /* scalar + int/float-array knobs (auto-generated from knobs.def) */
#define DEF_KNOB(opt, name, type, parser, defval) \
  else if (MATCH("", #opt))                       \
  {                                               \
    knob::name = get_##parser(value);             \
  }
#include "knobs.def"
#undef DEF_KNOB

  /* prefetcher type lists (accumulated across repeated options) */
  else if (MATCH("", "l1d_prefetcher_types")) {
    knob::l1d_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "l2c_prefetcher_types")) {
    knob::l2c_prefetcher_types.push_back(string(value));
  } else if (MATCH("", "llc_prefetcher_types")) {
    knob::llc_prefetcher_types.push_back(string(value));
  }

  else {
    printf("unable to parse section: %s, name: %s, value: %s\n", section, name,
           value);
    return 0;
  }
  return 1;
}

int handler(void *user, const char *section, const char *name,
            const char *value)
{
  char config_file_name[MAX_LEN];

  if (MATCH("", "config")) {
    strcpy(config_file_name, value);
    parse_config(config_file_name);
  } else {
    apply_knob(user, section, name, value);
  }
  return 1;
}

void parse_config(char *config_file_name)
{
  cout << "parsing config file: " << string(config_file_name) << endl;
  if (ini_parse(config_file_name, apply_knob, NULL) < 0) {
    printf("Failed to load %s\n", config_file_name);
    exit(1);
  }
}

void parse_knobs(int argc, char *argv[])
{
  for (int index = 0; index < argc; ++index) {
    string arg = string(argv[index]);
    if (arg.compare(0, 2, "--") == 0) {
      arg = arg.substr(2);
    }
    if (ini_parse_string(arg.c_str(), handler, NULL) < 0) {
      printf("error parsing commandline %s\n", argv[index]);
      exit(1);
    }
  }
}

// --------------------------------------------------------------------------
// Post-parse validation and derived-knob computation.
// --------------------------------------------------------------------------

// Invariants the original code asserted inline while parsing. Run once after
// all knobs are parsed, so the checks are independent of argument order and
// only fire for knobs that were actually provided.
void validate_knobs()
{
  if (!knob::rob_partition_size.empty()) {
    assert(knob::rob_partition_size.size() == knob::num_rob_partitions);
    int32_t len = 0;
    for (uint32_t i = 0; i < knob::rob_partition_size.size(); ++i) {
      len += knob::rob_partition_size[i];
    }
    assert(len == ROB_SIZE);
  }
  for (uint32_t i = 0; i < knob::rob_frontal_partition_ids.size(); ++i) {
    assert(knob::rob_frontal_partition_ids[i] <
           (int32_t)knob::num_rob_partitions);
  }
  for (uint32_t i = 0; i < knob::rob_dorsal_partition_ids.size(); ++i) {
    assert(knob::rob_dorsal_partition_ids[i] <
           (int32_t)knob::num_rob_partitions);
  }
}

// Recompute each derived knob from its parent. Derived knobs hold don't-care
// defaults (see knobs.def); their real value is set here after parsing.
void compute_derived_knobs()
{
  // ROB partition boundaries = prefix sums of the partition sizes.
  knob::rob_partition_boundaries.clear();
  int32_t len = 0;
  for (uint32_t i = 0; i + 1 < knob::rob_partition_size.size(); ++i) {
    len += knob::rob_partition_size[i];
    knob::rob_partition_boundaries.push_back(len);
  }
  knob::sms_region_size_log = log2(knob::sms_region_size);
  knob::scooby_max_states   = pow(2.0, knob::scooby_state_num_bits);
  knob::scooby_max_actions  = knob::scooby_actions.size();
  // Guarded only to avoid the undefined 1 << (negative) when dspatch is unused.
  if (knob::dspatch_log2_region_size != 0) {
    knob::dspatch_num_cachelines_in_region =
        1 << (knob::dspatch_log2_region_size - 6);  // 64B cachelines
  }
}

// Entry point: parse all knobs, then validate, then compute derived knobs.
void parse_args(int argc, char *argv[])
{
  parse_knobs(argc, argv);
  validate_knobs();
  compute_derived_knobs();
}
