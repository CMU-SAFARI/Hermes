#ifndef RAND_REPL_H
#define RAND_REPL_H

// Named rand_repl, not random: inc/ is on -I, so inc/random.h would shadow
// the <random> that champsim.h includes.
#include <random>
#include "cache.h"
#include "cache_repl_base.h"

class RandRepl : public CacheReplBase
{
private:
  std::mt19937_64 engine;
  // find_victim outcome counts, not fills/evictions: a fill that cannot
  // proceed re-enters and counts again.
  uint64_t invalid_way_picks = 0;
  uint64_t random_picks      = 0;

public:
  RandRepl(string name) : CacheReplBase(name) {}
  ~RandRepl() {}

  // override fuctions
  void     print_config();
  void     initialize_replacement();
  void     update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way,
                                    uint64_t paddr, uint64_t PC,
                                    uint64_t victim_addr, uint32_t type,
                                    uint8_t hit);
  uint32_t find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set,
                       const BLOCK *current_set, uint64_t ip,
                       uint64_t full_addr, uint32_t type);
  void     dump_stats();
};

#endif /* RAND_REPL_H */
