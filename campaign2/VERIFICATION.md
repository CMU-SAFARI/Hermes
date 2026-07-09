# Campaign 2 — Multi-core Fixed-Window Verification (Task 4)

**Date:** 2026-07-09
**Repo:** `/home/rahbera/thesis/Hermes-c2`, branch `campaign2`
**Code version:** `d918e36` (full: `d918e36191d23dac0e36eb196fe689c2542ba5ea`)
**Reference binary provenance:** `/home/rahbera/thesis/Hermes` pinned at `b18be0c` (read-only; HEAD of that repo independently confirmed `b18be0c994928677230dc142b4b887df9c94b470` at verification time)
**Design spec under test:** `docs/superpowers/specs/2026-07-09-multicore-window-fix-design.md`

Binaries were built FRESH from `d918e36` for this pass (the binaries already present in `bin/` predated the final fix commit by ~34 minutes and were discarded/overwritten):

```
./build_champsim.sh glc multi multi multi multi 1 1 0
./build_champsim.sh glc multi multi multi multi 4 1 0
```

| Binary | SHA-256 |
|---|---|
| `bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch` | `38ed8207a8abc6eeddf95ef0129823fe01de97dd1e68ff5360aec59d296b8906` |
| `bin/glc-perceptron-no-multi-multi-multi-multi-4core-1ch` | `f2f818c059309d255ac015c590ff24ccae48620514ff94c46a39b3e03d9db487` |

Repo working tree was clean (`git status --short` empty) both before and after both builds — the build script's in-tree file swaps (`inc/defs.h`, `branch/branch_predictor.cc`, etc.) self-restore on completion.

`$UNCORE_ARGS` (shared by all criteria, resolved relative to `/home/rahbera/thesis/Hermes-c2`):
```
--llc_replacement_type=ship --config=./config/nopref.ini --num_rob_partitions=3 \
--rob_partition_size=64,128,320 --rob_frontal_partition_ids=0 --rob_dorsal_partition_ids=2 \
--config=./config/ocp_hermes.ini --offchip_pred_location=uncore --ocp_perc_use_physical_address=true \
--ocp_perc_activated_features=20,21,22 --ocp_perc_weight_array_sizes=8192,1024,4096 \
--ocp_perc_feature_hash_types=2,2,2 --ocp_perc_feature_region_size_log2s=21,21,21 \
--ocp_perc_activation_threshold=2 --ocp_perc_pos_train_thresh=8 --ocp_perc_neg_train_thresh=-21 \
--ocp_perc_page_buf_sets=32 --ocp_perc_page_buf_assoc=16 --config=./config/hermes_base.ini \
--ddrp_req_latency=1
```

Traces used (all present under `/home/rahbera/thesis/runs/test_suite/`, no substitutions needed): `462.libquantum-1343B`, `605.mcf_s-1536B`, `607.cactuBSSN_s-2421B`, `621.wrf_s-8065B`, `654.roms_s-1390B`.

---

## Criterion 1 — Rigorous 1-core identity, two traces, campaign-scale window

**Command** (run from `/home/rahbera/thesis/Hermes-c2`; each of the 4 simulator invocations run to completion — 10M warmup + 30M simulation instructions, no window shrinking):

```bash
T=/home/rahbera/thesis/runs/test_suite
for tr in 462.libquantum-1343B 621.wrf_s-8065B; do
  ./bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch \
    --warmup_instructions=10000000 --simulation_instructions=30000000 $UNCORE_ARGS \
    -traces $T/$tr.champsimtrace.xz > new_$tr.out 2>&1
  /home/rahbera/thesis/Hermes/bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch \
    --warmup_instructions=10000000 --simulation_instructions=30000000 $UNCORE_ARGS \
    -traces $T/$tr.champsimtrace.xz > ref_$tr.out 2>&1
  diff <(grep -vE "Last compiled|Simulation time|Heartbeat" ref_$tr.out) \
       <(grep -vE "Last compiled|Simulation time|Heartbeat" new_$tr.out) && echo "IDENTICAL: $tr"
done
```

**Output:**

```
=== diff for 462.libquantum-1343B ===
IDENTICAL: 462.libquantum-1343B
=== diff for 621.wrf_s-8065B ===
IDENTICAL: 621.wrf_s-8065B
```

Both `diff`s produced zero lines of output (exit 0) — every stat line byte-matches the pinned `b18be0c` reference after stripping wall-clock-only lines (`Last compiled`, `Simulation time`, `Heartbeat`). Raw (unfiltered) line counts also matched exactly: libquantum new=ref=1122 lines (43 wall-clock lines filtered from each); wrf new=ref=1146 lines.

**Verdict: PASS**

---

## Criterion 2 — Checkpoint fidelity + divergence proof (4-core)

**Command** (cores: 0=libquantum, 1=mcf, 2=cactuBSSN, 3=wrf; 1M warmup + 2M simulation instructions):

```bash
HERMES_CKPT_DEBUG=1 ./bin/glc-perceptron-no-multi-multi-multi-multi-4core-1ch \
  --warmup_instructions=1000000 --simulation_instructions=2000000 $UNCORE_ARGS \
  -traces $T/462.libquantum-1343B.champsimtrace.xz $T/605.mcf_s-1536B.champsimtrace.xz \
          $T/607.cactuBSSN_s-2421B.champsimtrace.xz $T/621.wrf_s-8065B.champsimtrace.xz > fid.out 2>&1
```

### 2a. Brief's literal checker — output is vacuous, root-caused

The brief's checker regex-matches final-dump lines shaped `Core_N_offchip_pred_(true_pos|false_pos|false_neg)` and diffs them against the first `CKPT` value recorded for that (core, stat) pair:

```python
import re
txt=open('fid.out').read()
ckpt={}
for m in re.finditer(r'^CKPT(\d) (\S+) (\d+)$', txt, re.M):
    ckpt.setdefault((m.group(1), m.group(2)), []).append(int(m.group(3)))
final={}
for m in re.finditer(r'^Core_(\d)_offchip_pred_(true_pos|false_pos|false_neg) (\d+)$', txt, re.M):
    final[(m.group(1), 'stats.offchip_pred.'+m.group(2))]=int(m.group(3))
bad=[k for k,v in final.items() if ckpt.get(k,[None])[0]!=v]
print('fidelity mismatches:', bad if bad else 'NONE')
```

**Output:**
```
ckpt entries collected: 88
final Core_N_offchip_pred_* entries found: 0
fidelity mismatches: NONE
```

**This "NONE" is vacuous** — `final` is empty, so nothing was actually compared. Root cause, confirmed by reading `src/offchip_pred.cc:87-111` and `src/main.cc:351-358`: `Core_N_offchip_pred_*` is only emitted by `O3_CPU::dump_stats_offchip_predictor()`, which `main.cc` calls **only when `--offchip_pred_location=core`**. Our config (matching the uncore-placement OCP-Hermes study this fork exists for) uses `--offchip_pred_location=uncore`, in which case `main.cc:356-357` instead calls `uncore.LLC.dump_stats_offchip_predictor()`, printing a single aggregate `LLC_offchip_pred_*` block (not per-core, and not one of the checkpointed `ooo_cpu[i].stats.offchip_pred.*` fields). So under this run's placement, the brief's named stat is checkpointed in memory but never printed per-core by name — the literal script has nothing to check.

### 2b. Corrected, non-vacuous fidelity check

The design doc's own acceptance criterion 2 (`docs/superpowers/specs/2026-07-09-multicore-window-fix-design.md`) states the real requirement generically: *"diff against the final dump's values for that core: must be identical"* — not restricted to the offchip_pred naming. Using the actual registration inventory in `src/main.cc` (`REG_CS` block, lines 1596–1626) cross-referenced against every place those same fields are unconditionally printed per-core (`print_core_roi_stats`, `print_branch_stats`, and the fault-counter dump — none gated by `offchip_pred_location`), I checked all 21 checkpointed-and-dumped raw fields per core: `stats.bubble.{called,rob_non_head,rob_head,went_offchip,went_offchip_rob_head,went_offchip_rob_non_head}`, `stats.ddrp.{total,issued[0],issued[1],dram_rq_full,dram_mshr_full}`, `total_branch_types[0..7]`, `major_fault`, `minor_fault` — 21 × 4 cores = 84 comparisons.

**Output:**
```
cores found: ['0', '1', '2', '3']
fields checked (cores x fields): 84
missing final-dump counterparts: NONE
fidelity mismatches: NONE
```

All 84 checkpoint-vs-final-dump comparisons matched exactly.

### Divergence proof

```
Warmup complete CPU 0 instructions: 1000000 cycles: 400052 (Simulation time: 0 hr 0 min 31 sec)
Warmup complete CPU 1 instructions: 1000000 cycles: 400052 (Simulation time: 0 hr 0 min 31 sec)
Warmup complete CPU 2 instructions: 1000000 cycles: 400052 (Simulation time: 0 hr 0 min 31 sec)
Warmup complete CPU 3 instructions: 1000000 cycles: 400051 (Simulation time: 0 hr 0 min 31 sec)

Finished CPU 2 instructions: 2000001 cycles: 1447748  cumulative IPC: 1.38146 (Simulation time: 0 hr 1 min 2 sec)   <- fastest (cactuBSSN)
Finished CPU 3 instructions: 2000004 cycles: 2269527  cumulative IPC: 0.88124 (Simulation time: 0 hr 1 min 18 sec)  (wrf)
Finished CPU 0 instructions: 2000003 cycles: 4517652  cumulative IPC: 0.44271 (Simulation time: 0 hr 2 min 2 sec)   (libquantum)
Finished CPU 1 instructions: 2000001 cycles: 20183460 cumulative IPC: 0.09909 (Simulation time: 0 hr 6 min 26 sec)  <- slowest (mcf)
```

CPU 2's "Finished" line (sim time 1:02) precedes the run's true end — CPU 1 doesn't finish until 6:26, a 5m24s / ~14x-cycle gap. **Cores that overran CPU 2's checkpoint: CPU 3, CPU 0, and CPU 1** (all three finish strictly after CPU 2; CPU 1/mcf overruns the most).

Direct proof CPU 2's live counters kept moving during that gap (i.e., restore was doing real work, not a no-op): CPU 2's own `Heartbeat` lines continue climbing long after its `Finished` line —

```
(line 577) Finished CPU 2 instructions: 2000001 cycles: 1447748  ...(Simulation time: 0 hr 1 min 2 sec)
(line 739) Heartbeat CPU 2 instructions: 27000005 cycles: 20564609 ... (Simulation time: 0 hr 6 min 26 sec)
```

CPU 2 retired **~25,000,004 additional instructions** after its own [W, W+S] window closed, to sustain memory contention for the still-running cores — exactly the contamination mechanism the design doc describes. Despite that, CPU 2's final dump matches its checkpoint exactly on all 21 tracked fields (part of the 84/84 match in 2b), proving the checkpoint/restore mechanism discarded that overrun rather than leaking it into the reported window.

**Verdict: PASS**

---

## Criterion 3 — Window determinism across two mixes

**Command** (wrf fixed as core 0; two different {core1,core2,core3} co-runner sets; run to full completion, no window shrinking):

```bash
for co in "605.mcf_s-1536B 607.cactuBSSN_s-2421B 654.roms_s-1390B" "462.libquantum-1343B 605.mcf_s-1536B 607.cactuBSSN_s-2421B"; do
  set -- $co
  ./bin/glc-perceptron-no-multi-multi-multi-multi-4core-1ch \
    --warmup_instructions=1000000 --simulation_instructions=2000000 $UNCORE_ARGS \
    -traces $T/621.wrf_s-8065B.champsimtrace.xz $T/$1.champsimtrace.xz $T/$2.champsimtrace.xz $T/$3.champsimtrace.xz \
    2>&1 | grep -E "(Warmup complete|Finished) CPU 0 "
done
```
(Mix 1 = wrf,mcf,cactuBSSN,roms; Mix 2 = wrf,libquantum,mcf,cactuBSSN. Each run was let run to true completion — several minutes, dominated by mcf's low IPC — not truncated.)

**Output:**
```
=== mix 1: wrf + 605.mcf_s-1536B + 607.cactuBSSN_s-2421B + 654.roms_s-1390B ===
Warmup complete CPU 0 instructions: 1000000 cycles: 545267 (Simulation time: 0 hr 0 min 31 sec)
Finished CPU 0 instructions: 2000004 cycles: 2700167 cumulative IPC: 0.74070 (Simulation time: 0 hr 1 min 30 sec)

=== mix 2: wrf + 462.libquantum-1343B + 605.mcf_s-1536B + 607.cactuBSSN_s-2421B ===
Warmup complete CPU 0 instructions: 1000000 cycles: 400063 (Simulation time: 0 hr 0 min 25 sec)
Finished CPU 0 instructions: 2000002 cycles: 2264399 cumulative IPC: 0.88324 (Simulation time: 0 hr 1 min 4 sec)
```

**Sub-check A — warmup boundary: PASS.** `Warmup complete CPU 0 instructions:` is exactly `1000000` in both mixes — mix-independent, matching Fix 1's hard-cap guarantee exactly.

**Sub-check B — finish boundary: FAIL (as literally specified).** `Finished CPU 0 instructions:` is `2000004` in mix 1 vs `2000002` in mix 2 — neither equals `2000000` exactly, and the two mixes do not produce identical `CPU 0` boundary lines, contradicting both the brief's stated expectation ("`Finished CPU 0 instructions: 2000000` exactly" / "identical CPU 0 boundary lines in both mixes") and the design doc's acceptance criterion 3 ("`finish_sim_instr` == S exactly, on every core in both runs").

**Root-cause analysis** (reading `src/main.cc:1896-1901` and `src/ooo_cpu.cc:2555-2565`):
- Unlike the warmup boundary, there is **no retire cap at simulation completion** — `ooo_cpu.cc` only guards retirement with `if (!warmup_complete[cpu] && num_retired >= warmup_instructions) return;`. No analogous guard exists for `simulation_instructions`. `finish_sim_instr = num_retired - begin_sim_instr` is measured *after* that cycle's retire bundle (width `RETIRE_WIDTH=6`, `inc/uarch/glc.h:124`) has already retired, so it can overshoot S by up to 5 instructions, and the exact overshoot depends on which cycle the crossing bundle lands on — which shifts with contention from different co-runners.
- **This overshoot is not new**: the pinned single-core `b18be0c` reference shows the identical mechanism — `621.wrf_s-8065B` overshoots its 30,000,000-instruction target by exactly +1 in *both* the new and reference single-core binaries (see Criterion 1 evidence: `Finished CPU 0 instructions: 30000001` in both), while `462.libquantum-1343B` overshoots by 0 in both. Fix 1 explicitly scopes the hard-cap to warmup only ("SINGLE-CORE keeps the legacy path verbatim" / multi-core retire cap is `#if NUM_CPUS > 1` and only checks `!warmup_complete[cpu]`); Fix 2 only checkpoints/restores stat *values*, it does not change *when* `simulation_complete[i]` fires. Neither fix claims to (or was designed to) make the finish-instruction count exact.
- **Reproducibility, not flakiness**: mix 1 was re-run independently and reproduced `Finished CPU 0 instructions: 2000004 cycles: 2700167 cumulative IPC: 0.74070` byte-for-byte (modulo wall clock). The variance is a deterministic function of the mix configuration (which co-runners, which core indices), not run-to-run nondeterminism.
- **Downstream impact is real but small and contention-dominated, not corruption**: re-running both mixes with `HERMES_CKPT_DEBUG=1` and diffing wrf's (core 0) `CKPT0` block shows the checkpointed stats *do* differ between mixes — e.g. `stats.bubble.called` 316490 (mix1) vs 314288 (mix2), `total_rob_occupancy_at_branch_mispredict` 156639 vs 148128 — but `branch_mispredictions` is identical (617 = 617) and `num_branch` differs by only 6 (82022 vs 82028), consistent with wrf's own instruction/branch stream being deterministic per se. The larger deltas track the ~19% cycle-count difference between the two runs (2700167 vs 2264399 cycles for the ~same instruction count) — i.e., they reflect genuine, intended cross-mix contention variation (the whole point of preserving contention past a core's own completion), not leakage from the few-instruction tail difference or a broken checkpoint.

**Verdict: FAIL** (strictly, against the brief's literal "exactly"/"identical" wording for the finish-instruction boundary). Sub-check A (warmup-side determinism, Fix 1's actual scope) passes cleanly; sub-check B (finish-side exactness) does not hold, because no symmetric retire-cap exists at the S boundary — this is a pre-existing ChampSim retire-bundle-granularity artifact (proven present, identically, in the pinned single-core reference), bounded by `RETIRE_WIDTH` (≤5 instructions out of 2,000,000, ≤0.00025%), deterministic per mix configuration, and distinct from the stat-contamination bug that Fix 2 targets and that Criteria 1/2 confirm is fixed. Whether this residual tail variance is acceptable for campaign2's purposes, or warrants a symmetric simulation-completion retire cap (mirroring Fix 1) as a follow-up, is an owner decision outside this task's scope (verification only, no code changes made).

---

## Criterion 4 — Deadlock-mute sanity (big speed spread)

The Step-2 run above (libquantum fast vs. mcf slow, 4 cores, `fid.out`) doubles as this check.

**Command:**
```bash
grep -c DEADLOCK fid.out
```

**Output:**
```
0
```

**Verdict: PASS**

---

## Overall

| Criterion | Verdict |
|---|---|
| 1. Rigorous 1-core identity (2 traces, 10M+30M) | PASS |
| 2. Checkpoint fidelity + divergence proof (4-core) | PASS |
| 3. Window determinism across two mixes | **FAIL** (finish-boundary sub-check; warmup-boundary sub-check passes — see root-cause analysis above) |
| 4. Deadlock-mute sanity | PASS |

**Overall: BLOCKED.** Criteria 1, 2, and 4 pass rigorously, including a corrected, non-vacuous version of the Criterion 2 fidelity check (the brief's literal script is vacuous under `offchip_pred_location=uncore` — see Criterion 2 notes) and a concrete divergence proof. Criterion 3 fails strictly: the warmup-side window boundary is exactly reproduced and mix-independent (Fix 1's actual guarantee, fully verified), but the finish-side instruction count (`Finished CPU 0 instructions:`) is neither exactly `2000000` nor identical between the two tested mixes (`2000004` vs `2000002`), because no retire cap exists at simulation completion (only at warmup). This is a bounded (≤5 instructions, `RETIRE_WIDTH`), deterministic-per-mix, pre-existing artifact — proven structurally identical (same mechanism) to legacy single-core behavior already present in the pinned `b18be0c` reference — not a new regression, and not a recurrence of the stat-contamination bug Fix 2 targets. It does, however, contradict the literal acceptance text in both the task brief and the owner-ratified design doc, so it is reported here as a failure rather than silently rounded to a pass. Recommend the owner either (a) relax the Criterion 3 spec wording to match Fix 1's actual (warmup-only) scope, or (b) add a symmetric simulation-completion retire cap mirroring Fix 1's warmup cap if exact finish-boundary determinism is required.

Per explicit task-scope instructions, this report was written ONLY to `/home/rahbera/thesis/runs/tuning/campaign2/VERIFICATION.md`; the brief's Step 5 (rsync to `/home/rahbera/thesis/Hermes-tuning/campaign2/`, commit, push) was intentionally NOT performed — `/home/rahbera/thesis/Hermes-tuning` was not touched. No code changes or commits were made in `/home/rahbera/thesis/Hermes-c2`; `/home/rahbera/thesis/Hermes` was only invoked read-only as the pinned reference binary.


## Criterion 3 — RESOLVED (post-fix rerun)

Fix commit 6319cea adds the finish-boundary retire cap (pause at exactly
W+S until the same-cycle completion check snapshots; simulation_complete
lifts the cap so the core keeps running for contention). Rerun evidence
(both mixes, 1M+2M, post-fix binary): all four cores show
'Warmup complete ... 1000000' and 'Finished ... 2000000' EXACTLY; zero
DEADLOCK; exit 0. 1-core byte gate re-verified empty. Fidelity spot-check:
CKPT2 num_retired 3000000 exact; CKPT2 num_branch 17806 == summed final
branch-type lines. Fix re-review: Approved (livelock/ordering/staleness/
1-core-leak/overrun risks all cleared by source tracing).
**Criterion 3: PASS** — full evidence in Hermes-c2/.superpowers/sdd/task-4-report-fix.md

## FINAL VERDICT: ALL FOUR CRITERIA PASS
Code version: campaign2 @ 6319cea. Binaries: 1core 9bbe0d3b188434fb, 4core ef3b36a358ca948d.

## INCIDENT ADDENDUM (2026-07-09, post-launch): warmup-barrier deadlock false-positive

At production 25M-instruction warmups, real mixes diverge ~3M cycles (the
short-window verification mixes diverged ~1 cycle — the triage's "parks
are ~1 cycle" premise did not scale). A parked core's completed ROB head
carried event_cycles stale by the park duration; the first post-unpark
deadlock check aborted 606/800 run01 jobs. Fix 0c4a1fa (per-core
deadlock_rearm_cycle set at the barrier): A/B-reproduced at a lowered
threshold, verified clean at the real threshold, 1-core byte gate empty.
Binary re-pinned a327a7cbb3504a59. Lesson recorded: boundary-hazard
findings triaged on "unreachable at current scale" must be re-checked at
production scale before launch.
