# Multi-core Fixed Instruction Windows — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every core in a multi-core run measures exactly instructions [W, W+S] of its own stream, mix-independent: cores park at their warmup target until all are warm, and all core-level stats checkpoint at each core's W+S and are restored before the final dump.

**Architecture:** Two independent mechanisms. (1) A compile-time-gated (`#if NUM_CPUS > 1`) warmup park: retire caps at exactly W, the per-core pipeline block in main.cc is skipped while parked, the deadlock check is muted; the existing all-warm barrier is untouched. (2) A checkpoint/restore registry (`CoreStatsCheckpoint`, new header): named scalars + custom save/restore closures, registered at the composition root in main.cc, checkpointed at each core's existing `simulation_complete[i]` site, restored once before the final dump so untouched print code emits window-correct values.

**Tech Stack:** C++11, existing Makefile via `./build_champsim.sh`. No unit-test framework exists — every task verifies with real simulator runs and byte/value diffs against the pinned `b18be0c` binary.

## Global Constraints

- Work ONLY in `/home/rahbera/thesis/Hermes-c2` (branch `campaign2`). Never touch `/home/rahbera/thesis/Hermes` (campaign-1 pin) or `/home/rahbera/thesis/Hermes-tuning`.
- Single-core (`NUM_CPUS == 1`) output must be BYTE-IDENTICAL to the b18be0c binary (modulo the `Last compiled` banner line). All new behavior sits behind `#if NUM_CPUS > 1` or is value-neutral at 1 core (checkpoint==final, restore==identity).
- No output-format changes, no new default output lines, no knobs. Debug fidelity output only under env var `HERMES_CKPT_DEBUG`.
- No changes to `reset_stats`, `dump_stats`, or any print function.
- Reference binaries: 1-core b18be0c = `/home/rahbera/thesis/Hermes/bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch` (campaign-1, do not rebuild it); local traces in `/home/rahbera/thesis/runs/test_suite/`.
- Build commands: 1-core `./build_champsim.sh glc multi multi multi multi 1 1 0`; 4-core `./build_champsim.sh glc multi multi multi multi 4 1 0` (run from Hermes-c2; libbf already present).
- The full campaign-1 uncore config used in verification runs (call it `$UNCORE_ARGS`):
  `--llc_replacement_type=ship --config=./config/nopref.ini --num_rob_partitions=3 --rob_partition_size=64,128,320 --rob_frontal_partition_ids=0 --rob_dorsal_partition_ids=2 --config=./config/ocp_hermes.ini --offchip_pred_location=uncore --ocp_perc_use_physical_address=true --ocp_perc_activated_features=20,21,22 --ocp_perc_weight_array_sizes=8192,1024,4096 --ocp_perc_feature_hash_types=2,2,2 --ocp_perc_feature_region_size_log2s=21,21,21 --ocp_perc_activation_threshold=2 --ocp_perc_pos_train_thresh=8 --ocp_perc_neg_train_thresh=-21 --ocp_perc_page_buf_sets=32 --ocp_perc_page_buf_assoc=16 --config=./config/hermes_base.ini --ddrp_req_latency=1`
- Commit style: repo Lore format (see .claude/CLAUDE.md); run `.claude/skills/git-commit/format.sh --fix` before committing C++ changes; no AI co-author lines.

**Task dependency graph:** Task 1 ∥ Task 2 (different files — parallelizable). Task 3 after both (touches main.cc, consumes Task 1's API). Task 4 after 3. Task 5 after 4.

---

### Task 1: CoreStatsCheckpoint registry header

**Files:**
- Create: `inc/core_stats_checkpoint.h`

**Interfaces:**
- Produces (Task 3 relies on these exact names):
  `class CoreStatsCheckpoint` with
  `void reg(uint32_t cpu, const char *name, uint64_t *p)`,
  `void reg_custom(uint32_t cpu, const char *name, std::function<void()> save, std::function<void()> restore)`,
  `void checkpoint(uint32_t cpu)`, `void restore_all()`.
  Global instance `extern CoreStatsCheckpoint core_stats_ckpt;` (defined in main.cc by Task 3).

- [ ] **Step 1: Write the header**

```cpp
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
```

- [ ] **Step 2: Syntax-check the header standalone**

Run:
```bash
cd /home/rahbera/thesis/Hermes-c2 && \
echo '#include "inc/core_stats_checkpoint.h"
CoreStatsCheckpoint core_stats_ckpt;
int main(){ static uint64_t x=7; core_stats_ckpt.reg(0,"x",&x); core_stats_ckpt.checkpoint(0); x=99; core_stats_ckpt.restore_all(); return (x==7)?0:1; }' > /tmp/ckpt_test.cc && \
g++ -std=c++11 -I./inc -I./libbf /tmp/ckpt_test.cc -o /tmp/ckpt_test && /tmp/ckpt_test && echo REGISTRY-OK
```
Expected: `REGISTRY-OK` (exit 0: scalar restored 99→7).

- [ ] **Step 3: Commit**

```bash
cd /home/rahbera/thesis/Hermes-c2 && bash .claude/skills/git-commit/format.sh --fix
git add inc/core_stats_checkpoint.h
git commit -m "Add per-core stat checkpoint/restore registry (header only)

Named-scalar fast path + custom save/restore closures; checkpoint(cpu) at
each core's window end, restore_all() before the final dump. Debug
fidelity print only under HERMES_CKPT_DEBUG (no default output).

Constraint: no output/format/knob changes; value-neutral at 1 core.
Confidence: high
Scope-risk: narrow
Tested: standalone host test (register/checkpoint/mutate/restore).
Not-tested: integration (Task 3)."
```

---

### Task 2: Warmup park (exact stop at W), multi-core only

**Files:**
- Modify: `src/ooo_cpu.cc` (retire_rob loop head, ~line 2553)
- Modify: `src/main.cc` (helper + pipeline gate ~1591, deadlock check ~1665, warmup flip ~1674)

**Interfaces:**
- Consumes: nothing from other tasks.
- Produces: `static inline bool warmup_parked(int i)` in main.cc (Task 3 does not use it; listed for reviewer orientation).

- [ ] **Step 1: Retire cap in `src/ooo_cpu.cc`**

At the very top of `void O3_CPU::retire_rob()`'s `for (uint32_t n = 0; n < RETIRE_WIDTH; n++) {` loop body, insert:

```cpp
#if NUM_CPUS > 1
    // Multi-core fixed windows: park retirement at exactly the warmup
    // target so every core's measured region starts at W (spec:
    // docs/superpowers/specs/2026-07-09-multicore-window-fix-design.md).
    if (!warmup_complete[cpu] && num_retired >= warmup_instructions) {
      return;
    }
#endif
```

- [ ] **Step 2: Helper + gates in `src/main.cc`**

(a) Near the other file-scope statics (above the simulation loop), add:

```cpp
#if NUM_CPUS > 1
// Multi-core fixed windows: a core that has finished its own warmup parks
// (no pipeline progress) until ALL cores are warm. Caches/DRAM keep
// operating so in-flight transactions drain. Zero code at 1 core.
static inline bool warmup_parked(int i)
{
  return warmup_complete[i] && (all_warmup_complete <= NUM_CPUS);
}
#else
static inline bool warmup_parked(int) { return false; }
#endif
```

(b) The pipeline block gate — change
```cpp
      if (stall_cycle[i] <= current_core_cycle[i]) {
```
to
```cpp
      if (!warmup_parked(i) && (stall_cycle[i] <= current_core_cycle[i])) {
```

(c) Mute the deadlock check for parked cores — change
```cpp
      if (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].ip &&
          (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle +
           DEADLOCK_CYCLE) <= current_core_cycle[i]) {
        print_deadlock(i);
      }
```
to
```cpp
      if (!warmup_parked(i) && ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].ip &&
          (ooo_cpu[i].ROB.entry[ooo_cpu[i].ROB.head].event_cycle +
           DEADLOCK_CYCLE) <= current_core_cycle[i]) {
        print_deadlock(i);
      }
```

(d) The warmup flip — change
```cpp
      if ((warmup_complete[i] == 0) &&
          (ooo_cpu[i].num_retired > ooo_cpu[i].warmup_instructions)) {
```
to
```cpp
#if NUM_CPUS > 1
      // capped cores reach exactly W and must still flip (>=); the 1-core
      // path keeps the legacy strict-> to preserve byte-identity
      if ((warmup_complete[i] == 0) &&
          (ooo_cpu[i].num_retired >= ooo_cpu[i].warmup_instructions)) {
#else
      if ((warmup_complete[i] == 0) &&
          (ooo_cpu[i].num_retired > ooo_cpu[i].warmup_instructions)) {
#endif
```

- [ ] **Step 3: Build 1-core and byte-verify vs b18be0c**

```bash
cd /home/rahbera/thesis/Hermes-c2 && ./build_champsim.sh glc multi multi multi multi 1 1 0 >/tmp/c2_1c_build.log 2>&1 && echo BUILD-OK
T=/home/rahbera/thesis/runs/test_suite
ARGS="--warmup_instructions=2000000 --simulation_instructions=5000000 $UNCORE_ARGS"
./bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch $ARGS -traces $T/462.libquantum-1343B.champsimtrace.xz > /tmp/new_1c.out 2>&1
/home/rahbera/thesis/Hermes/bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch $ARGS -traces $T/462.libquantum-1343B.champsimtrace.xz > /tmp/ref_1c.out 2>&1
diff <(grep -v "Last compiled" /tmp/ref_1c.out) <(grep -v "Last compiled" /tmp/new_1c.out) && echo BYTE-IDENTICAL
```
(Substitute `$UNCORE_ARGS` from Global Constraints, with `./config/` paths.)
Expected: `BYTE-IDENTICAL` (zero diff lines).

- [ ] **Step 4: Build 4-core and verify exact-W park**

```bash
cd /home/rahbera/thesis/Hermes-c2 && ./build_champsim.sh glc multi multi multi multi 4 1 0 >/tmp/c2_4c_build.log 2>&1 && echo BUILD-OK
T=/home/rahbera/thesis/runs/test_suite
./bin/glc-perceptron-no-multi-multi-multi-multi-4core-1ch --warmup_instructions=500000 --simulation_instructions=500000 $UNCORE_ARGS \
  -traces $T/462.libquantum-1343B.champsimtrace.xz $T/605.mcf_s-1536B.champsimtrace.xz $T/607.cactuBSSN_s-2421B.champsimtrace.xz $T/621.wrf_s-8065B.champsimtrace.xz > /tmp/park_4c.out 2>&1
grep "Warmup complete CPU" /tmp/park_4c.out
grep -c "DEADLOCK" /tmp/park_4c.out || true
```
Expected: all four `Warmup complete CPU i instructions: 500000` — EXACTLY 500000 on every core; zero DEADLOCK lines; run exits 0.

- [ ] **Step 5: Commit**

```bash
cd /home/rahbera/thesis/Hermes-c2 && bash .claude/skills/git-commit/format.sh --fix
git add src/ooo_cpu.cc src/main.cc
git commit -m "Park cores at exactly their warmup target until all are warm

Multi-core only (#if NUM_CPUS > 1): retire caps at W, the parked core's
pipeline block is skipped (caches/DRAM keep draining), the deadlock check
is muted while parked, and the warmup flip becomes >= for capped cores.
Single-core keeps the legacy path verbatim.

Constraint: 1-core output byte-identical to b18be0c (verified, libquantum
2M+5M full uncore config).
Rejected: fetch-gate-and-drain (nondeterministic W+delta start).
Confidence: high
Scope-risk: narrow
Tested: 1-core byte-diff clean; 4-core park shows Warmup complete == W
exactly on all cores, no spurious deadlock prints.
Not-tested: checkpoint integration (Task 3+)."
```

---

### Task 3: Registration, checkpoint, restore integration

**Files:**
- Modify: `src/main.cc` (define global; registration block after per-cpu init; checkpoint call at simulation_complete; restore_all after the sim loop)

**Interfaces:**
- Consumes from Task 1: `core_stats_ckpt.reg / reg_custom / checkpoint / restore_all` exactly as declared.
- Produces: complete per-core stat coverage (audit table in the commit message).

- [ ] **Step 1: Include + define the global in `src/main.cc`**

With the other includes: `#include "core_stats_checkpoint.h"`.
Near the other globals (e.g., below `warmup_complete` declaration block):
```cpp
CoreStatsCheckpoint core_stats_ckpt;
```

- [ ] **Step 2: Registration block (composition root)**

Insert immediately BEFORE `print_knobs();` (after all per-cpu init, line ~1570):

```cpp
  // Multi-core fixed windows: register every core-level statistic for
  // checkpoint at that core's own [W, W+S] end (registration lives here at
  // the composition root, NOT in constructors, so shared structures are
  // never mis-scoped; LLC per-cpu slices register under their cpu).
  for (uint32_t i = 0; i < NUM_CPUS; i++) {
    O3_CPU *c = &ooo_cpu[i];
#define REG_CS(field) core_stats_ckpt.reg(i, #field, &c->field)
    REG_CS(stats.bubble.called);
    REG_CS(stats.bubble.rob_non_head);
    REG_CS(stats.bubble.rob_head);
    REG_CS(stats.bubble.went_offchip);
    REG_CS(stats.bubble.went_offchip_rob_head);
    REG_CS(stats.bubble.went_offchip_rob_non_head);
    REG_CS(stats.offchip_pred.pred_called);
    REG_CS(stats.offchip_pred.true_pos);
    REG_CS(stats.offchip_pred.false_pos);
    REG_CS(stats.offchip_pred.false_neg);
    REG_CS(stats.ddrp.total);
    REG_CS(stats.ddrp.issued[0]);
    REG_CS(stats.ddrp.issued[1]);
    REG_CS(stats.ddrp.dram_rq_full);
    REG_CS(stats.ddrp.dram_mshr_full);
    REG_CS(num_branch);
    REG_CS(branch_mispredictions);
    REG_CS(total_rob_occupancy_at_branch_mispredict);
    for (uint32_t b = 0; b < 8; b++) {
      core_stats_ckpt.reg(i, "total_branch_types", &c->total_branch_types[b]);
    }
#undef REG_CS
    // bubble per-partition vectors + per-partition load stats + per-IP maps
    core_stats_ckpt.reg_custom(
        i, "bubble_vectors",
        [c]() {
          c->bubble_max_ckpt = c->bubble_max;
          c->bubble_min_ckpt = c->bubble_min;
          c->bubble_tot_ckpt = c->bubble_tot;
          c->bubble_cnt_ckpt = c->bubble_cnt;
        },
        [c]() {
          c->bubble_max = c->bubble_max_ckpt;
          c->bubble_min = c->bubble_min_ckpt;
          c->bubble_tot = c->bubble_tot_ckpt;
          c->bubble_cnt = c->bubble_cnt_ckpt;
        });
    core_stats_ckpt.reg_custom(
        i, "load_per_ip_maps",
        [c]() {
          c->load_per_ip_stats_ckpt         = c->load_per_ip_stats;
          c->frontal_load_per_ip_stats_ckpt = c->frontal_load_per_ip_stats;
        },
        [c]() {
          c->load_per_ip_stats         = c->load_per_ip_stats_ckpt;
          c->frontal_load_per_ip_stats = c->frontal_load_per_ip_stats_ckpt;
        });
    core_stats_ckpt.reg_custom(
        i, "load_per_rob_part",
        [c]() {
          memcpy(c->load_per_rob_part_stats_ckpt, c->load_per_rob_part_stats,
                 sizeof(c->load_per_rob_part_stats));
        },
        [c]() {
          memcpy(c->load_per_rob_part_stats, c->load_per_rob_part_stats_ckpt,
                 sizeof(c->load_per_rob_part_stats));
        });
  }
```

This requires shadow members on O3_CPU (Modify `inc/ooo_cpu.h`, right after the originals):
```cpp
  // shadow copies for the multi-core stat checkpoint (see
  // core_stats_checkpoint.h); written only by the registry closures
  vector<uint64_t> bubble_max_ckpt, bubble_min_ckpt, bubble_tot_ckpt,
      bubble_cnt_ckpt;
  unordered_map<uint64_t, load_per_ip_info_t> load_per_ip_stats_ckpt,
      frontal_load_per_ip_stats_ckpt;
  load_per_rob_part_info_t
      load_per_rob_part_stats_ckpt[NUM_PARTITION_TYPES];
```

- [ ] **Step 3: AUDIT (mandatory, recorded in the commit)**

Enumerate every per-core value the final dump prints and confirm coverage:
```bash
cd /home/rahbera/thesis/Hermes-c2
grep -nE '"Core_" << cpu|Core_" << i|ooo_cpu\[(cpu|i)\]\.' src/main.cc | sed -n '1,200p'
grep -nE "print_sim_stats|print_roi_stats|dump_stats" src/main.cc | head
```
For EACH printed per-core value, record in the audit table: registered scalar / custom / cache-sim custom (Step 4) / already-frozen (`finish_sim_*`, roi) / exempt-shared (reason). If a value is found uncovered (e.g., a stat family this plan missed), ADD its registration in the same style before proceeding. The completed table goes in the Task-6-style commit message of Step 7.

- [ ] **Step 4: Cache sim-stat slices (private caches, TLBs, LLC per-cpu slice)**

The final dump prints cache `sim_*` (and TLB) counters from LIVE arrays — contaminated by overrun today. Register each core's slices as one custom per core, inside the same registration loop (after Step 2's customs). First inspect the CACHE stat arrays:
```bash
grep -nE "sim_access|sim_hit|sim_miss|roi_access" inc/cache.h | head
```
Then register (adjust field list to EXACTLY what `print_sim_stats(cpu, cache)` reads — the audit in Step 3 governs; the pattern below covers the classic trio across all access types):
```cpp
    {
      CACHE *caches[] = {&c->ITLB, &c->DTLB, &c->STLB,
                         &c->L1I,  &c->L1D,  &c->L2C,  &uncore.LLC};
      for (CACHE *ch : caches) {
        core_stats_ckpt.reg_custom(
            i, "cache_sim_slice",
            [ch, i]() {
              for (uint32_t t = 0; t < NUM_TYPES; t++) {
                ch->sim_access_ckpt[i][t] = ch->sim_access[i][t];
                ch->sim_hit_ckpt[i][t]    = ch->sim_hit[i][t];
                ch->sim_miss_ckpt[i][t]   = ch->sim_miss[i][t];
              }
            },
            [ch, i]() {
              for (uint32_t t = 0; t < NUM_TYPES; t++) {
                ch->sim_access[i][t] = ch->sim_access_ckpt[i][t];
                ch->sim_hit[i][t]    = ch->sim_hit_ckpt[i][t];
                ch->sim_miss[i][t]   = ch->sim_miss_ckpt[i][t];
              }
            });
      }
    }
```
with shadow arrays added to `inc/cache.h` next to the originals:
```cpp
  // shadows for the multi-core stat checkpoint (core_stats_checkpoint.h)
  uint64_t sim_access_ckpt[NUM_CPUS][NUM_TYPES] = {},
           sim_hit_ckpt[NUM_CPUS][NUM_TYPES]    = {},
           sim_miss_ckpt[NUM_CPUS][NUM_TYPES]   = {};
```
If the audit (Step 3) shows the dump reads additional per-cpu cache fields (e.g., prefetch counters indexed by cpu), extend both shadow arrays and closures identically.

- [ ] **Step 5: Checkpoint + restore call sites in `src/main.cc`**

(a) In the simulation-complete block, immediately after the `record_roi_stats(...)` calls:
```cpp
        core_stats_ckpt.checkpoint(i);
```
(b) Immediately AFTER the `while (run_simulation)` loop closes (before any end-of-run printing):
```cpp
  // restore each core's checkpointed [W, W+S] stats so the (untouched)
  // final dump prints window-correct values
  core_stats_ckpt.restore_all();
```

- [ ] **Step 6: Rebuild both, re-verify 1-core byte-identity, 4-core smoke**

Repeat Task 2 Step 3 byte-diff exactly (expected: `BYTE-IDENTICAL`) and Task 2 Step 4 run (expected: completes, exact-W lines still present). Additionally:
```bash
HERMES_CKPT_DEBUG=1 ./bin/glc-perceptron-no-multi-multi-multi-multi-4core-1ch \
  --warmup_instructions=500000 --simulation_instructions=500000 $UNCORE_ARGS \
  -traces <same 4 traces> > /tmp/ckpt_dbg.out 2>&1
grep -c "^CKPT" /tmp/ckpt_dbg.out
```
Expected: CKPT lines = 4 cores × (26 scalars + 3 customs + 7 cache customs) = 4 × 36 = 144 (adjust if the audit added registrations); ZERO `CKPT` lines when the env var is unset.

- [ ] **Step 7: Commit (with the completed audit table in the message body)**

```bash
cd /home/rahbera/thesis/Hermes-c2 && bash .claude/skills/git-commit/format.sh --fix
git add inc/ooo_cpu.h inc/cache.h src/main.cc
git commit  # Lore format; body MUST contain the audit table from Step 3
```

---

### Task 4: Full verification suite (spec acceptance criteria 1–4)

**Files:**
- Create: `/home/rahbera/thesis/runs/tuning/campaign2/VERIFICATION.md` (report; campaign artifact, not committed to the code repo)

**Interfaces:** consumes the binaries built in Task 3.

- [ ] **Step 1: Criterion 1 — rigorous 1-core identity, two traces, campaign-scale window**

```bash
T=/home/rahbera/thesis/runs/test_suite; cd /home/rahbera/thesis/Hermes-c2
for tr in 462.libquantum-1343B 621.wrf_s-8065B; do
  ./bin/*1core-1ch --warmup_instructions=10000000 --simulation_instructions=30000000 $UNCORE_ARGS -traces $T/$tr.champsimtrace.xz > /tmp/new_$tr.out 2>&1
  /home/rahbera/thesis/Hermes/bin/*1core-1ch --warmup_instructions=10000000 --simulation_instructions=30000000 $UNCORE_ARGS -traces $T/$tr.champsimtrace.xz > /tmp/ref_$tr.out 2>&1
  diff <(grep -vE "Last compiled|Simulation time|Heartbeat" /tmp/ref_$tr.out) <(grep -vE "Last compiled|Simulation time|Heartbeat" /tmp/new_$tr.out) && echo "IDENTICAL: $tr"
done
```
Expected: `IDENTICAL` for both. (Heartbeat/Simulation-time lines carry wall-clock text; all stat lines must byte-match. If ANY stat line differs → STOP, diagnose, fix before proceeding.)

- [ ] **Step 2: Criterion 2 — checkpoint fidelity + divergence proof (4-core)**

```bash
HERMES_CKPT_DEBUG=1 ./bin/*4core-1ch --warmup_instructions=1000000 --simulation_instructions=2000000 $UNCORE_ARGS \
  -traces $T/462.libquantum-1343B.champsimtrace.xz $T/605.mcf_s-1536B.champsimtrace.xz $T/607.cactuBSSN_s-2421B.champsimtrace.xz $T/621.wrf_s-8065B.champsimtrace.xz > /tmp/fid.out 2>&1
python3 - <<'EOF'
import re
txt=open('/tmp/fid.out').read()
ckpt={}
for m in re.finditer(r'^CKPT(\d) (\S+) (\d+)$', txt, re.M):
    ckpt.setdefault((m.group(1), m.group(2)), []).append(int(m.group(3)))
# final dump values for the offchip_pred raws per core
final={}
for m in re.finditer(r'^Core_(\d)_offchip_pred_(true_pos|false_pos|false_neg) (\d+)$', txt, re.M):
    final[(m.group(1), 'stats.offchip_pred.'+m.group(2))]=int(m.group(3))
bad=[k for k,v in final.items() if ckpt.get(k,[None])[0]!=v]
print('fidelity mismatches:', bad if bad else 'NONE')
EOF
```
Expected: `fidelity mismatches: NONE`. Divergence proof: the fastest core's checkpoint moment appears well before the run end (confirm its `Finished CPU` line precedes others), i.e., its live counters continued past the checkpointed values before restore.

- [ ] **Step 3: Criterion 3 — window determinism across two mixes**

Run wrf as core-0 workload in two different mixes; both runs must show `Warmup complete CPU 0 instructions: 1000000` exactly and `Finished CPU 0 instructions: 2000000` exactly:
```bash
for co in "605.mcf_s-1536B 607.cactuBSSN_s-2421B 654.roms_s-1390B" "462.libquantum-1343B 605.mcf_s-1536B 607.cactuBSSN_s-2421B"; do
  set -- $co
  ./bin/*4core-1ch --warmup_instructions=1000000 --simulation_instructions=2000000 $UNCORE_ARGS \
    -traces $T/621.wrf_s-8065B.champsimtrace.xz $T/$1.champsimtrace.xz $T/$2.champsimtrace.xz $T/$3.champsimtrace.xz 2>&1 | grep -E "(Warmup complete|Finished) CPU 0 "
done
```
Expected: identical `CPU 0` boundary lines in both mixes.

- [ ] **Step 4: Criterion 4 — deadlock-mute sanity (big speed spread)**

The Step-2 run (libquantum fast vs mcf slow) doubles as this check: `grep -c DEADLOCK /tmp/fid.out` → expected `0`.

- [ ] **Step 5: Write the verification report + publish**

`/home/rahbera/thesis/runs/tuning/campaign2/VERIFICATION.md`: one section per criterion with the exact commands, outputs, and PASS/FAIL. Then rsync campaign2 dir to `/home/rahbera/thesis/Hermes-tuning/campaign2/`, commit there ("campaign2: window-fix verification report"), push origin tuning.

---

### Task 5: Re-pin and record

- [ ] **Step 1:** Rebuild the quad-core binary from the final `campaign2` HEAD (`./build_champsim.sh glc multi multi multi multi 4 1 0`); run the two Task-2/Task-3 smokes once more on the final binary.
- [ ] **Step 2:** `sha256sum bin/glc-perceptron-no-multi-multi-multi-multi-4core-1ch` → update `runs/tuning/campaign2/PLAN.md`: replace pin `52357d378d1a036f` with the new sha[:16], note the campaign2 HEAD commit, mark Deliverable 1 re-pinned post-window-fix.
- [ ] **Step 3:** Update campaign-1 `state.json` log with one entry (campaign-2 window fix landed + new pin) and publish campaign2 dir via the Hermes-tuning worktree; push.

## Self-review notes
- Spec coverage: Fix-1 → Task 2; Fix-2 registry → Tasks 1+3; audit table → Task 3 Steps 3/7; criteria 1–4 → Task 4; criterion 5 (re-pin) → Task 5. No gaps found.
- The exact CKPT line count in Task 3 Step 6 depends on audit additions — stated as adjustable, not a placeholder.
- Type consistency: `reg(uint32_t, const char*, uint64_t*)` used identically in Tasks 1 and 3; all inventoried loose scalars are uint64_t (verified against inc/ooo_cpu.h).
