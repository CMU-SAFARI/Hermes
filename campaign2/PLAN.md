# Campaign 2 — quad-core validation of Hermes-Big/Normal/Lite (STAGING)

Successor to campaign1 (single-core tuning).
OWNER AMENDMENT 2026-07-09: uncore predictor stats remain SHARED (no
per-core attribution — pooled LLC_offchip_pred_* is the P/R signal, with
its full-phase semantics documented). The solo-vs-mix per-workload P/R
comparison is DROPPED. Campaign 2's goal is the PERFORMANCE-ISOLATION
study: per-core IPC (and all per-core counters) are exactly window-scoped
[W, W+S] and mix-independent thanks to the fixed-window platform, so
in-mix slowdown vs solo IPC, the Big/Normal/Lite gap under contention,
and the embed_cpu_id on/off A-B are all cleanly measurable.

## Deliverable 1 — quad-core binary: DONE, PINNED
- Worktree: /home/rahbera/thesis/Hermes-c2 (branch campaign2 @ b18be0c) —
  fully isolated from campaign1's frozen checkout.
- Build: ./build_champsim.sh glc multi multi multi multi 4 1 0
- Binary: bin/glc-perceptron-no-multi-multi-multi-multi-4core-1ch
- RE-PINNED 2026-07-09 after the fixed-window changes: sha256[:16] ef3b36a358ca948d
  (campaign2 @ 6319cea; supersedes 52357d378d1a036f). FIXED for all of
  campaign 2. Windows are now exact per core: every core measures
  [W, W+S] of its own stream regardless of co-runners (warmup park +
  finish-boundary cap + full per-core stat checkpoint/restore; all four
  acceptance criteria PASS — see VERIFICATION.md).
- libbf dependency copied from the main tree (git-ignored vendored lib).
- Smokes passed: (a) 4-core baseline w/ real args; (b) shared uncore
  predictor, Hermes-Normal config, embed_cpu_id=true — 113k predictions,
  per-core IPCs sane.
- KNOWN PAPERCUT (pre-existing, not fixed — observed at 4-core default
  config): running WITHOUT --llc_replacement_type=ship segfaults in
  llc_replacement_print_config(). All experiments must pass ship (they
  always have).

## Deliverable 2 — experiment file (PENDING owner input)
Smaller per-core warmup/sim windows (owner). Open: exact window sizes.

## Deliverable 3 — mixed-trace tlist (PENDING owner input)
Open: mix construction (homogeneous vs heterogeneous, how many mixes, which
traces — presumably sampled from the 146-trace list), solo-reference runs
(each trace alone on the 4-core binary? or reuse campaign1 single-core data
— NOTE: solo references must come from the SAME binary/config for a clean
in-mix-vs-solo comparison; propose 4-core binary with 3 idle cores? No —
standard practice: solo = the workload on core 0 of the quad-core system
with other cores idle is not supported by ChampSim traces; typical method:
solo = single-core-equivalent run; DISCUSS with owner).

## Decisions closed 2026-07-09
- DRAM channels: 1 channel for the quad-core (owner-confirmed; pinned binary stands).
- Shipping configs frozen as config/hermes_uncore_{big,normal,lite}.ini —
  committed on rbdev (fc49264, the architect handover artifacts) and
  cherry-picked to campaign2 (8ad4b78); exp_quad.yml now references the
  ini files instead of spelling knobs. Validated: all three parse through
  the pinned 1-core binary; 4-core smoke through the lite ini shows exact
  200000-instruction warmup boundaries on all cores.
- Uncore predictor stats: remain shared (owner amendment; see above).

## Open decisions for the owner
1. DRAM channels: built with 1 channel (max contention — matches the
   precision-under-pressure story). Say the word for a 2-channel variant
   BEFORE the campaign pins its protocol (rebuild is 2 minutes; both
   binaries could even coexist, names differ).
2. Mix design + count; per-core window sizes; embed on/off A-B matrix.
3. Campaign-2 cluster setup: needs its own remote_sim_path + bootstrap so
   cluster builds use the 4-core build command (campaign1's .cluster-run
   config builds 1-core — MUST NOT be reused).
