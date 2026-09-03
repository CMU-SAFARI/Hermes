# Design: "blind" bandwidth-gated DDRP baseline

Date: 2026-08-04
Owner: Rahul Bera
Status: design approved (Approach A) — pending implementation plan

## 1. Motivation

The architects want justification for the hardware complexity of Hermes's
perceptron off-chip predictor (POPET: page buffer + weight tables + training).
This experiment builds the strongest *no-predictor* baseline: on **every demand-
load L2 miss**, proactively fire a direct-DRAM read (DDRP) **whenever DRAM
bandwidth is below a threshold** — no page buffer, no weights, no training. If
this "blind" mechanism matches Hermes's speedup at comparable DRAM traffic, the
predictor's complexity is not justified; if it needs far more traffic for equal
or less speedup, the predictor earns its keep (precision saves bandwidth).

## 2. Key code facts (verified 2026-08-04)

- **Uncore DDRP hook** — `src/cache.cc:1043-1054`: on an L2 read miss, when the
  uncore predictor exists (`offchip_pred_location=="uncore"`) and
  `rq_entry.is_data && rq_entry.type == LOAD`, it calls
  `uncore.LLC.offchip_pred->predict(&rq_entry)`; on a positive prediction and
  `knob::enable_ddrp`, it calls `uncore.LLC.issue_ddrp_request(&rq_entry)`. So
  the predictor already fires on exactly Hermes's target population (demand-load
  L2 misses) — the blind predictor at the same site is apples-to-apples.
- **Predictor factory** — `src/offchip_pred.cc:23` `create_offchip_predictor`
  switches on `offchip_pred_type` (`none`/`basic`/`random`/`perc`/`xpt`/...). An
  uncore allow-list asserts only `none`/`xpt`/`perc` may live at the LLC — extend
  it to include `blind`.
- **DRAM bandwidth signal** — the DRAM controller already quantizes bandwidth
  into `DRAM_BW_LEVELS = 4` levels per epoch (`inc/commons.h:99`;
  `inc/dram_controller.h:46-51` bw accounting). Pythia consumes it
  (`prefetcher/scooby.cc:874-898`, gating on `bw_level >= scooby_high_bw_thresh`).
  Global `uncore.DRAM` (`inc/uncore.h:29`) is queryable from the uncore predictor.

## 3. Mechanism (Approach A — new predictor type, reuse DDRP)

1. **`OffchipPredBlind : OffchipPredBase`** (new, ~30 lines):
   - `predict(...)` → `return uncore.DRAM.get_current_bw_level() < knob::blind_ddrp_bw_thresh;`
     (fire the DDRP iff current DRAM bandwidth level is below the threshold).
   - `train(...)` is a no-op. **No page buffer, no weights, no history — zero
     predictor state.** That absence *is* the baseline.
2. **`get_current_bw_level()`** — a small getter on the DRAM controller exposing
   the per-epoch bandwidth level it already computes (added if not already
   present).
3. **Knob** `blind_ddrp_bw_thresh` (uint32, **default 0**, sweepable
   `0..DRAM_BW_LEVELS`): fire iff `current_bw_level < thresh`, where level **0 =
   lowest** utilization and `DRAM_BW_LEVELS-1` (=3) = highest. So `thresh=0` never
   fires (safe no-op default; the experiment always sets it explicitly),
   `thresh=1` fires only at the lowest-bandwidth level, `thresh=4` always fires.
   Threshold levels map to the ~25/50/75/100%-of-peak boundaries. Default 0 keeps
   any unset run inert — and no existing config selects `offchip_pred_type=blind`
   anyway, so this adds zero risk to the frozen results.
4. **Factory + allow-list**: register `"blind"` in `create_offchip_predictor`
   and add it to the uncore allow-list assertion.
5. **No changes to the DDRP hook or the DDRP path.** The blind predictor plugs in
   like `perc`/`xpt`; `issue_ddrp_request` and the `dram_controller` DDRP path are
   reused verbatim.

Rejected alternatives: (B) modifying the shared DDRP path in `cache.cc` directly
— more invasive, muddies the clean "swap the predictor" comparison. (C) a
bandwidth-admission drop inside the DRAM controller — only needed if we want a
per-channel gate at issue time; the predict-time gate is simpler and sufficient.

## 4. Gate semantics (decided)

- Metric: **% of peak over a recent epoch**, realized via the existing 4-level
  quantized signal (minimal, matches Pythia's infra, thresholds at ~25/50/75%).
- Location: **predict-time** read of the current bandwidth level (contained in
  the predictor; the DDRP is never created when bandwidth is high).
- If finer resolution is later wanted, compute a continuous `%util` in the DRAM
  controller and expose it — a drop-in change to the getter; not needed now.

## 5. Experiment configuration

Same chain as Hermes-UnC, swapping the predictor:

```
WBASE + hermes_base.ini --ddrp_req_latency=1 \
  --offchip_pred_type=blind --offchip_pred_location=uncore \
  --blind_ddrp_bw_thresh=T
```

- `enable_ddrp` comes from `hermes_base.ini`; `ddrp_req_latency=1` matches
  Hermes-UnC for a fair action-latency comparison.
- The blind predictor ignores the address (no `ocp_hermes.ini`/perc config).

## 6. Comparison matrix & metrics

- Configs (146 mem-intensive traces, full window, DDR-3200):
  `nopref`, `Hermes-UnC` (perc), and `blind` at T ∈ {1,2,3,4}.
- Metrics: **speedup over nopref**; **% DRAM-read-traffic increase over nopref**
  (the decisive one — blind should flood DRAM); Hermes precision/recall for
  context (blind has no accuracy). Reuse the existing membound146 rollup/plot
  tooling.

## 7. Success criteria

1. `blind` builds and runs; `blind_ddrp_bw_thresh` measurably changes how often
   the DDRP fires and the resulting DRAM traffic.
2. Core-mode regression unaffected (this is uncore-only; `perc`/core paths
   untouched) — run the regression to confirm no core-mode drift.
3. The sweep answers the architect question: does blind approach Hermes's speedup,
   and at what DRAM-traffic cost?

## 8. Files touched (estimate)

- new `inc/offchip_pred_blind.h`, `src/offchip_pred_blind.cc`
- `src/offchip_pred.cc` (factory + allow-list)
- `inc/knobs.def` (`blind_ddrp_bw_thresh`)
- `inc/dram_controller.h` / `src/dram_controller.cc` (`get_current_bw_level()` getter if absent)
- build wiring (Makefile object list) as needed

## 9. Future extension (Experiment 2 — separate plan, NOT built here)

A follow-on experiment: **row-buffer-open-only prefetch.** For every demand-load
L2 miss, issue a DRAM request that **activates (opens) the target row but does no
column read** — no data transfer, no data-bus occupancy, no MSHR fill, no return
packet. Payoff: a later demand access to that row is a **row-buffer hit** (saves
tRCD / a conflict precharge), cutting effective DRAM latency **without consuming
read bandwidth** — the key contrast with Experiment 1. Costs: activation energy
and possible row-buffer contention.

ChampSim has no ACT-only request type today (a normal access does
ACT+CAS+data-return); Experiment 2 will add a DRAM request variant that stops
after the activation. **Design implication kept for Experiment 1:** the blind
predictor here is *action-agnostic* (it only decides "fire the off-chip action").
Experiment 2 reuses this predictor and adds a new DRAM **action** (row-open)
selected by a knob — no predictor change. Do NOT build the action abstraction in
Experiment 1; just keep the predictor decoupled from read semantics (it already
is). Full plan to be written separately after Experiment 1 completes.
