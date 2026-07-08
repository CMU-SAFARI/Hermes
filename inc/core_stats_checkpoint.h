#ifndef CORE_STATS_CHECKPOINT_H
#define CORE_STATS_CHECKPOINT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <vector>
#include "defs.h"

// Per-core statistics checkpoint/restore (multi-core fixed windows).
// At each core's simulation-complete point, checkpoint(cpu) copies every
// registered value into a shadow store; restore_all() writes the shadows
// back before the final dump so untouched print code emits values from the
// core's own [W, W+S] window. See
// docs/superpowers/specs/2026-07-09-multicore-window-fix-design.md.
class CoreStatsCheckpoint
{
private:
  struct scalar_t {
    const char *name;
    uint64_t   *ptr;
    uint64_t    shadow;
  };
  struct custom_t {
    const char           *name;
    std::function<void()> save;
    std::function<void()> restore;
  };
  std::vector<scalar_t> scalars[NUM_CPUS];
  std::vector<custom_t> customs[NUM_CPUS];
  bool                  done[NUM_CPUS] = {};

public:
  void reg(uint32_t cpu, const char *name, uint64_t *p)
  {
    scalars[cpu].push_back({name, p, 0});
  }
  void reg_custom(uint32_t cpu, const char *name, std::function<void()> save,
                  std::function<void()> restore)
  {
    customs[cpu].push_back({name, save, restore});
  }
  void checkpoint(uint32_t cpu)
  {
    bool dbg = (getenv("HERMES_CKPT_DEBUG") != NULL);
    for (auto &s : scalars[cpu]) {
      s.shadow = *s.ptr;
      if (dbg) {
        printf("CKPT%u %s %lu\n", cpu, s.name, (unsigned long)s.shadow);
      }
    }
    for (auto &c : customs[cpu]) {
      c.save();
      if (dbg) {
        printf("CKPT%u %s saved\n", cpu, c.name);
      }
    }
    done[cpu] = true;
  }
  void restore_all()
  {
    for (uint32_t c = 0; c < NUM_CPUS; ++c) {
      if (!done[c]) {
        continue;
      }
      for (auto &s : scalars[c]) {
        *s.ptr = s.shadow;
      }
      for (auto &cu : customs[c]) {
        cu.restore();
      }
    }
  }
};

extern CoreStatsCheckpoint core_stats_ckpt;

#endif /* CORE_STATS_CHECKPOINT_H */
