#include "rand_repl.h"

extern uint64_t
    champsim_seed;  // deterministic, derived from the trace path in main.cc

void RandRepl::print_config()
{
  cout << "rand.seed " << champsim_seed << endl;
}

// Seeded here rather than in the ctor so the value is champsim_seed's, which
// main.cc assigns before the LLC is built.
void RandRepl::initialize_replacement()
{
  engine.seed(champsim_seed);
}

// find replacement victim
uint32_t RandRepl::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set,
                               const BLOCK *current_set, uint64_t ip,
                               uint64_t full_addr, uint32_t type)
{
  // fill invalid line first, matching lru_victim
  for (uint32_t way = 0; way < LLC_WAY; way++) {
    if (current_set[way].valid == false) {
      invalid_way_picks++;
      return way;
    }
  }

  random_picks++;
  // Plain modulo, not uniform_int_distribution: the distribution's algorithm
  // is unspecified, so it can differ across libstdc++ and break run-to-run
  // reproducibility across machines. mt19937_64 itself is standard-defined.
  return (uint32_t)(engine() % LLC_WAY);
}

// called on every cache hit and cache fill; random keeps no per-line state
void RandRepl::update_replacement_state(uint32_t cpu, uint32_t set,
                                        uint32_t way, uint64_t full_addr,
                                        uint64_t ip, uint64_t victim_addr,
                                        uint32_t type, uint8_t hit)
{
}

void RandRepl::dump_stats()
{
  cout << "rand.find_victim.invalid_way " << invalid_way_picks << endl
       << "rand.find_victim.random_pick " << random_picks << endl;
}
