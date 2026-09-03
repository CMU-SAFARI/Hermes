# Design: Knobbed Uncore (beside-LLC) Call Path for the Off-chip Predictor

## Goal

The off-chip load predictor is logically inside the core today: `predict()` is called at
load-queue insertion ([ooo_cpu.cc:1354](../src/ooo_cpu.cc#L1354), pre-translation) and
`train()` at LQ release ([ooo_cpu.cc:2691](../src/ooo_cpu.cc#L2691)). To study an
industrial **uncore/beside-LLC** placement (where the PC and core structures aren't
available, but the **physical** address is), we add an alternative call path that
predicts when a data-LOAD misses L2 and reaches the LLC, and trains when the LLC
hit/miss is resolved.

This is a **knobbed alternative**, not a replacement: `offchip_pred_location = {core, uncore}`.
In `core` mode the simulator runs exactly today's code. In `uncore` mode it exercises the
new call points and a new `PACKET*`-based predict/train. **No existing code is deleted.**
Scope of this change: relocate predict/train and measure accuracy/coverage at the uncore;
the speculative-fetch *action* (DDRP/LLC-bypass) is explicitly out of scope here.

## Decisions locked

- **Additive overloads.** Keep `predict/train(ooo_model_instr*, uint32_t, LSQ_ENTRY*)`
  byte-for-byte for the core path. **Add** `predict/train(PACKET*)` across base + all
  derived classes for the uncore path. Calls go through an `OffchipPredBase*`, so virtual
  dispatch picks the right overload (no name-hiding issue for base-pointer calls).
- **Predictor lives where it is used (no aliasing).** In `uncore` mode the single predictor
  is owned by the LLC (an `offchip_pred` pointer on the LLC `CACHE`/`UNCORE`); the per-core
  `ooo_cpu[i].offchip_pred` pointers stay **NULL** and are never used. In `core` mode it is
  the reverse: each core owns its instance and the LLC pointer is NULL. So exactly one side
  holds a valid pointer, selected by `offchip_pred_location`. `packet->cpu` is still used
  inside the LLC-owned predictor to reduce cross-core interference.
- **Off-chip = LLC miss.** Label comes from the LLC tag probe (`check_hit() < 0`),
  including MSHR-merged secondary misses. Filter to demand data loads only
  (`cache_type == IS_LLC && is_data && type == LOAD`).

## Overview of planned changes

```
knob offchip_pred_location ─┐
                            ├─ core  → existing call sites (1354 predict / 2691 train), unchanged
                            └─ uncore→ new hooks in CACHE::handle_read() at the LLC
PACKET gains: ocp_feature*, went_offchip      (additive; carries state+label predict→train)
OffchipPredBase + 9 derived gain: predict(PACKET*) / train(PACKET*)   (additive overloads)
ownership: core mode -> per-core ooo_cpu[i].offchip_pred ; uncore mode -> LLC.offchip_pred (cores NULL)
stats: TP/FP/FN attributed to ooo_cpu[packet->cpu].stats.offchip_pred at the LLC train hook
```

## Brief description of each change

1. **Knob** `offchip_pred_location` (string, default `"core"`) — declared/parsed in
   [knobs.cc](../src/knobs.cc) like `offchip_pred_type`; echoed in `print_config_offchip_predictor`.
2. **PACKET** ([block.h](../inc/block.h) `class PACKET`) — add `ocp_base_feature_t* ocp_feature`
   (init NULL) and `uint8_t went_offchip` (init 0). `went_offchip_pred` already exists. These
   let the uncore path carry feature-state and the outcome label on the request itself.
3. **Predictor interface** — in [offchip_pred_base.h](../inc/offchip_pred_base.h)/.cc add
   `virtual bool predict(PACKET*)` (default: `return false`) and `virtual void train(PACKET*)`
   (default: no-op). Declare the overrides across all derived headers; implement bodies where
   we evaluate (below). Predictors that don't override inherit the base default in uncore mode.
4. **Ownership (LLC-owned in uncore mode, no aliasing)** — give the LLC `CACHE`/`UNCORE` its
   own `OffchipPredBase* offchip_pred` (init NULL). In `uncore` mode, construct the predictor
   once into the LLC's pointer (a new LLC-side init, e.g. `uncore.LLC.initialize_offchip_predictor`);
   the per-core `ooo_cpu[i].offchip_pred` pointers remain NULL (their `initialize_offchip_predictor`
   is already gated to `core`). In `core` mode it is unchanged: cores construct their instances and
   the LLC pointer stays NULL. Exactly one side is valid.
5. **Uncore call points** — in [cache.cc](../src/cache.cc) `CACHE::handle_read()` (~L833), only when
   `cache_type == IS_LLC`, `location == uncore`, and the entry `is_data && type == LOAD`. Calls go
   through the **LLC's own** `offchip_pred` (i.e. `this->offchip_pred`), not `ooo_cpu[...]`:
   - **Predict** when the read is serviced, **before** consuming the hit result, storing the
     returned prediction into `rq_entry.went_offchip_pred` and feature-state into
     `rq_entry.ocp_feature`: `this->offchip_pred->predict(&rq_entry)`.
   - **Train** right after `check_hit()` resolves: set `rq_entry.went_offchip = (way < 0)`,
     call `this->offchip_pred->train(&rq_entry)`, then free `rq_entry.ocp_feature`.
     This covers both outcomes (hit ⇒ not off-chip, miss ⇒ off-chip) in one pass.
6. **Stats attribution** — reuse the TP/FP/FN logic from
   [`offchip_pred_stats_and_train`](../src/ooo_cpu.cc#L2663): in the uncore train hook, update
   `ooo_cpu[rq_entry.cpu].stats.offchip_pred.{true_pos,false_pos,false_neg}` from
   `went_offchip` vs `went_offchip_pred` (per-core reporting keeps working even though the
   predictor itself is LLC-owned). The LLC-owned predictor's internal `dump_stats()` is printed
   once from the uncore (not per core).
7. **Core path gating** — wrap the two existing core call sites (1354, 2691) so they fire only
   when `location == core`. Nothing else in the core path changes.
8. **New predict/train bodies — XPT only** ([offchip_pred_xpt.cc](../src/offchip_pred_xpt.cc)):
   extract the physical page from `packet->full_addr` (genuinely available at the LLC). Shared
   `predict_helper`/`train_helper` back both placements. Other predictors (POPET/perc, basic,
   hmp-\*, ttp) inherit the base default and are deferred to **step 11**.

## Step-by-step plan

1. ✅ **[DONE]** Add the `offchip_pred_location` knob (declare + parse + print).
2. ✅ **[DONE]** Add `ocp_base_feature_t*` and `went_offchip` to PACKET (additive, initialized).
3. ✅ **[DONE]** Add `predict(PACKET*)` / `train(PACKET*)` to the base class with safe defaults
   (derived classes inherit the defaults until their bodies are implemented in step 8).
4. ✅ **[DONE]** Gate the existing core call sites (1354/2691) behind `location == core`.
5. ✅ **[DONE]** **Regression checkpoint #1**: golden before-vs-after over the 5-trace suite
   showed `OK: no change` on IPC/precision/recall — steps 1–4 are inert on the core path.
   (Required first making SHiP deterministic, committed `f5b7d85`.)
6. ✅ **[DONE]** Uncore predictor ownership: the LLC owns its `offchip_pred` (constructed under
   `location == uncore`); per-core pointers stay NULL (no aliasing). All derefs NULL-guarded;
   an invariant assert enforces exactly one of cores/LLC owns the predictor. Factory logs
   `Adding Offchip predictor: <type> at <core/LLC>` and asserts LLC supports only `none`~
   until the uncore bodies land (step 8).
7. ✅ **[DONE]** New uncore call points: **predict** at the L2→LLC handoff (in `handle_read`'s
   miss path, right before `lower_level->add_rq`, gated to `cache_type==IS_L2C` + uncore + demand
   data-LOAD) using the LLC-owned predictor — as early as possible, hiding LLC RQ queuing latency;
   the prediction + `ocp_feature` ride on the PACKET into the LLC RQ. **Train** at the LLC after
   `check_hit` (`went_offchip = way<0`) via `CACHE::offchip_pred_stats_and_train`, which does the
   per-cpu TP/FP/FN attribution, trains, and frees `ocp_feature`. Per-core `Core_*_offchip_pred_*`
   stats now print in both modes. Verified with `none` at LLC: FN = all LLC-missing data loads.
8. ✅ **[DONE]** Implement the new `PACKET*` bodies for **XPT** (physical-page from
   `packet->full_addr`), via shared `predict_helper`/`train_helper`; LLC-owned accuracy stats
   (`LLC_offchip_pred_*`); LLC-support assert relaxed to admit `xpt`. Committed `8afa015`.
   (Other predictors deferred to step 11.)
9. ✅ **[DONE]** **Regression checkpoint #2**: core POPET over the 5-trace suite == `det1`
   golden (`OK: no change`) — the uncore work (steps 6–8) is inert on the core path.
   On 462.libquantum (DDRP off): XPT@uncore+physical 99.5%/48.6% vs XPT@core+virtual 24.5%/44.4%.
10. **Add the DDRP (speculative direct-DRAM) path for XPT at the uncore.** *(not defined yet)*
   Today the uncore prediction is observational only — predict/train fire and we measure
   accuracy/coverage, but a positive prediction issues no early DRAM request, so the core-side
   DDRP action (`issue_ddrp_request`) is bypassed. Until we define the uncore speculative-fetch
   path (issue the DRAM request when XPT predicts off-chip, then satisfy the LLC miss from it),
   the predictor's coverage does **not** convert into IPC gains. This is the milestone that turns
   the 48.6% coverage into actual speedup.
11. **Add uncore-side support for more off-chip predictor types** (e.g. POPET/perc and the
   PC-less feature families). Implement their `PACKET*` bodies and relax the LLC-support assert
   accordingly. POPET uncore needs the PC-less, physical-address feature set
   ([pcless-offchip-features.md](pcless-offchip-features.md)).

## Key reuse / integration points

- LLC identity: `cache_type == IS_LLC` (set in [main.cc:1285](../src/main.cc#L1285)); LLC read handling
  and hit/miss decision in `CACHE::handle_read()`/`check_hit()` ([cache.cc:833,1599](../src/cache.cc#L833)).
- PACKET physical address: `full_addr` (full PA) and `address` (block PA); `ip`, `type`, `is_data`,
  `cpu`, `went_offchip_pred` already present ([block.h](../inc/block.h)).
- Feature-state pattern mirrors `LSQ_ENTRY::ocp_feature` ([block.h:583](../inc/block.h#L583)) and its
  allocate-in-predict / read-in-train / free-on-release lifecycle.
- Core off-chip label today is set in `send_signal_to_core()` ([cache.cc:~2499](../src/cache.cc#L2499)) on
  LLC miss — same semantics our uncore label uses, computed locally from `check_hit()`.

## Out of scope (flag for later)

- The speculative direct-DRAM fetch / LLC-bypass action (`issue_ddrp_request`,
  [ooo_cpu.cc:2694](../src/ooo_cpu.cc#L2694)) stays core-side and is not wired to the uncore
  prediction yet — uncore mode only measures accuracy/coverage.
- Multi-core stat semantics with a shared instance (single-core runs are unaffected).

## Verification

1. **Build**: `./build_champsim.sh glc multi multi multi multi 1 1 0` (clean compile of base + all
   9 derived with the new overloads), then refresh the timestamped binary in `runs/bin/`.
2. **Core-mode regression**: run `runs/test.sh` (XPT) and the POPET config in `core` mode; confirm
   `Core_0_offchip_pred_*` stats are **identical** to the pre-change runs (proves core path untouched).
3. **Uncore functional**: rerun with `--offchip_pred_location=uncore`. For XPT, also set
   `ocp_xpt_use_physical_address=true` (now valid). Confirm the predictor fires (non-zero predicts)
   and report precision/recall.
4. **Compare**: uncore-XPT vs core-XPT vs core-POPET on 462.libquantum; sanity-check that uncore
   predictions key on physical pages (e.g. tracker hit/insertion counts are sane).
