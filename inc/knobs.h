#ifndef KNOBS_H
#define KNOBS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#define MAX_LEN 256
using namespace std;

namespace knob
{
#define DEF_KNOB(opt, name, type, parser, defval) extern type name;
#include "knobs.def"
#undef DEF_KNOB

/* hand-written knobs: accumulating prefetcher type lists */
extern vector<string> l1d_prefetcher_types;
extern vector<string> l2c_prefetcher_types;
extern vector<string> llc_prefetcher_types;
}  // namespace knob

void parse_args(int argc, char *argv[]);
void parse_knobs(int argc, char *argv[]);
int  apply_knob(void *user, const char *section, const char *name,
                const char *value);
int  handler(void *user, const char *section, const char *name,
             const char *value);
void parse_config(char *config_file_name);
void validate_knobs();
void compute_derived_knobs();
void print_knobs();

/* parser helpers */
int32_t         get_int32(const char *str);
uint32_t        get_uint32(const char *str);
uint64_t        get_uint64(const char *str);
bool            get_bool(const char *str);
string          get_string(const char *str);
float           get_float(const char *str);
double          get_double(const char *str);
uint32_t        get_hex32(const char *str);
uint64_t        get_hex64(const char *str);
vector<int32_t> get_int32v(const char *str);
vector<float>   get_floatv(const char *str);

#endif /* KNOBS_H */
