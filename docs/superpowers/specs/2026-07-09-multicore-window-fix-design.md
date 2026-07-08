# Multi-core fixed instruction windows: warmup-stall + per-core stat checkpoint

Owner-ratified 2026-07-09 (Rahul Bera). Branch: `campaign2` in the Hermes-c2
worktree only; campaign-1's `b18be0c` pin is untouched. Semantics are
UNCONDITIONAL (no knob). Print/reset/output-format changes are explicitly
deferred (owner decision) — this spec covers checkpoint/restore only.

## Problem

ChampSim's phase machinery makes a core's measured instruction window depend
on its co-runners:

1. `warmup_complete[i]` flips per-core at its warmup target (main.cc:1674),
   but `finish_warmup()` — which captures `begin_sim_instr = num_retired` and
   resets stats — runs only when ALL cores are warm (main.cc:1679, :923).
   Fast cores keep retiring meanwhile, so their measured window starts at
   `W + drift_i`, with `drift_i` set by the slowest co-runner.
2. After a core finishes its `simulation_instructions`, it keeps running (to
   preserve contention — desired) but only cache ROI stats are snapshotted
   (`record_roi_stats`). Every other per-core counter — including
   `Core_N_offchip_pred_*` (precision/recall raws), branch stats, TLB stats,
   and the per-IP maps — keeps accumulating and is printed contaminated.

Consequence: the same workload measures a different program slice in every
mix, and in-mix vs solo comparisons (campaign-2's central criterion) are
confounded. After this fix, a run with warmup W and simulation S always
measures instructions [W, W+S] of every workload, mix-independent.

## Fix 1 — warmup-stall (exact stop at W)

- MULTI-CORE ONLY (`if (NUM_CPUS > 1)` — a compile-time constant, so this is
  structural specialization, not a runtime knob): during warmup, the retire
  stage caps its bundle so `num_retired` never exceeds
  `warmup_instructions`: retire at most
  `min(RETIRE_WIDTH, warmup_instructions - num_retired)` while
  `warmup_complete[i] == 0`, and the warmup flip condition for the
  multi-core path is `num_retired >= warmup_instructions` (the capped core
  reaches exactly W and must still flip; the legacy strict-`>` would never
  fire against a capped counter). The core therefore halts at exactly W.
- SINGLE-CORE keeps the legacy path verbatim (no cap, strict-`>` flip):
  today's 1-core `begin_sim_instr` is W+epsilon (bundle overshoot), and
  preserving byte-identity with the b18be0c binary requires preserving that
  epsilon. The [W, W+S] exactness guarantee is a multi-core property, which
  is the only place it matters (solo/single-core runs are self-consistent).
- Stall condition: `stalled(i) := warmup_complete[i] && all_warmup_complete
  <= NUM_CPUS`. While stalled, the main loop skips core i's pipeline-stage
  calls (trace read / fetch / decode / schedule / execute / retire and any
  per-cycle core bookkeeping that advances architectural state), and the
  deadlock check for core i is muted (a parked ROB head is not a deadlock).
- Core i's PRIVATE CACHES (and TLBs) continue to operate while stalled so
  in-flight MSHR transactions drain; the shared LLC/DRAM obviously continue.
  A stalled core simply stops injecting new work.
- The global barrier `finish_warmup()` is UNCHANGED (latency swap, stat
  reset, `begin_sim_instr`/`begin_sim_cycle` capture). It now fires with
  every core parked at exactly W, so `begin_sim_instr == W` for all cores by
  construction. Timing during warmup is meaningless (owner: structures
  population only), so neither the stall nor the missing traffic from
  stalled cores is a concern.
- Single-core: with the legacy path preserved verbatim (above) and the
  stall/barrier logic firing in the same loop iteration, the change is a
  structural no-op at NUM_CPUS=1 — verified byte-identically, not assumed
  (see Verification). Implementation invariant for the stall gate: a stalled
  core's `num_retired`, ROB/LQ/SQ occupancy, and trace position must not
  change while stalled; the exact set of gated calls is determined during
  implementation against that invariant.

## Fix 2 — per-core stat checkpoint/restore registry

New, small, self-contained component (one header, e.g.
`inc/core_stats_checkpoint.h`), plus a registration block at system-assembly
time in main.cc (composition root — NOT in class constructors, so shared
structures are never mis-scoped as core stats):

API:
- `reg(cpu, &counter)` — scalar fast path; overloads for
  `uint64_t*/uint32_t*/int64_t*/float*/double*`.
- `reg_custom(cpu, save_fn, restore_fn)` — closure pair for non-scalars
  (the per-IP `unordered_map`s: deep copy on save, swap/assign on restore).
- `checkpoint(cpu)` — copies every registered item's current value into the
  shadow store. Called at the existing `simulation_complete[i]` site,
  adjacent to `record_roi_stats`.
- `restore_all()` — writes every shadow value back over the live counters.
  Called exactly once, when `all_simulation_complete == NUM_CPUS`, BEFORE
  the final stat dump. All existing print code then emits window-correct
  values without modification.

Registration inventory (audit families; the implementation MUST complete
the table by enumerating every per-core value read by the final dump and
recording its registration or its exemption + reason in the commit):

| family | storage | mechanism |
|--------|---------|-----------|
| core stat struct (bubble, ROB-partition arrays, offchip_pred TP/FP/FN, DDRP-attributed, etc.) | `ooo_cpu[i].stats.*` | reg() per scalar / array loop |
| branch stats | `num_branch`, `branch_mispredictions`, `total_branch_types[8]`, `total_rob_occupancy_at_branch_mispredict` | reg() |
| per-IP maps | `load_per_ip_stats`, `frontal_load_per_ip_stats` | reg_custom deep copy |
| TLB stats | `ITLB/DTLB/STLB` (CACHE objects) | extend existing `record_roi_stats` calls (3 additions) — one mechanism per stat family |
| private caches + LLC per-cpu slices | existing | UNCHANGED (`record_roi_stats`, already correct) |
| window bounds / IPC | `finish_sim_instr/cycle` | UNCHANGED (already frozen at completion) |

Out of scope (owner deferral, revisit later): auto-print unification, reset
unification, any output-name/format change, SHARED-scope stats (LLC
aggregates, DRAM, shared-OCP internals `ocp_perc_*` keep full-phase
semantics; documented as diagnostics-only — everything decision-grade is
per-core attributed and covered above).

## Verification (acceptance criteria; all must pass before re-pin)

1. **Single-core value-identity (rigorous).** Build 1-core from `campaign2`;
   run vs the pinned `b18be0c` 1-core binary on at least TWO traces
   (462.libquantum + 621.wrf) with the full campaign-1 uncore config, 10M+30M.
   Parse both outputs into name→value maps: every stat name present in both
   with EXACTLY equal values; name sets equal. Any diff = fail. (Value-level
   equivalence, since formatting is untouched, should in fact be byte-equal
   except the build-timestamp banner — check byte-diff first, fall back to
   map compare only for known-volatile lines.)
2. **Checkpoint fidelity at 4 cores.** Debug-print each registered value at
   `checkpoint(i)` time; after the run, diff against the final dump's values
   for that core: must be identical, while at least one live counter
   demonstrably advanced during the overrun (proves the restore protected
   something real).
3. **Window determinism.** Same workload placed in two different mixes:
   `Warmup complete CPU i instructions:` == W exactly, `finish_sim_instr`
   == S exactly, on every core in both runs.
4. **Deadlock-mute sanity.** A mix with a large warmup-speed spread runs
   without spurious DEADLOCK prints during the stall phase.
5. Rebuild quad-core, smoke (baseline + shared-OCP embed config), then
   **re-pin** (new sha replaces 52357d37 in runs/tuning/campaign2/PLAN.md).

## Risks / notes

- The stall gate must skip ONLY pipeline progress, not the memory system:
  a hard-frozen L2 with a live LLC strands returns. Gate placement is the
  main implementation risk; the drain behavior is asserted by watching MSHR
  occupancy reach zero for stalled cores in the fidelity test.
- `current_core_cycle[i]` continues advancing for stalled cores (heartbeats
  stay alive; warmup cycle counts are meaningless anyway).
- The retire-cap must not alter single-core retire bundling BELOW the W
  boundary (cap = min(width, W - retired) only while warmup incomplete).
- Restore overwrites live counters at end-of-run; nothing reads live values
  after the final dump, but the debug fidelity print (verification 2) must
  run BEFORE restore_all().
