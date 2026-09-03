# Deadlock-watchdog failures — running log

Running log of runs killed by ChampSim's **deadlock watchdog**, kept so future
debugging can start from a concrete repro instead of from scratch. These have
occurred historically at a low rate and are **not** treated as a correctness
problem for aggregate results; the affected runs are just dropped (`trace_failed`)
in rollup. Each entry is dated and left `Open` until investigated.

## Background: what the watchdog is

ChampSim aborts when the ROB head fails to retire for `DEADLOCK_CYCLE` (1,000,000)
cycles, via `print_deadlock()` ([../src/main.cc:1034](../src/main.cc#L1034),
`Assertion '0' failed`, process exit code **6**, `Aborted (core dumped)`). On abort
it dumps the stalled instruction, the Load Queue, and the cache MSHRs to stdout
(`.out`); the assert message lands in `.err`. The watchdog is a *symptom* detector,
not the bug — it tells you an in-flight request never completed, not why.

Status legend: `Open` · `In progress` · `Done`

---

## DL-1 — spec26 XPT-at-uncore batch (9/600 runs)

- **Added:** 2026-06-24
- **Status:** Open
- **Severity:** low (1.5% of the batch; known historical failure class)

### Context

- **Cluster batch:** `20260624T192220Z_spec26_xpt_uncore` on kratos2.
- **Run dir:** `/home/rahbera/from-rnadig/runs/Hermes/20260624T192220Z_spec26_xpt_uncore`
  (per-run `<trace>_<exp>.out` / `.err`).
- **Binary:** `glc-perceptron-no-multi-multi-multi-multi-1core-1ch`, built from `rbdev`
  @ `0decb3d` (i.e. *after* the clang-format + X-macro knob-framework refactors).
- **Workload:** 4 experiments × 150 spec26 v2 traces, 100M warmup / 500M sim. All four
  experiments run the off-chip predictor at the **uncore with the DDRP action enabled**:
  - `xpt_uncore_o` / `xpt_uncore_p` = `BASE` + `ocp_xpt.ini`
    `--offchip_pred_location=uncore --ocp_xpt_use_physical_address=true`
    + `hermes_base.ini` (`enable_ddrp=true`) `--ddrp_req_latency=1` / `=8`.
  - `pythia_with_xpt_uncore_o/p` = the same, plus Pythia (`scooby`) at the L2.

### The failures (all identical)

| Job ID | Trace | Variants | Elapsed at abort |
|---|---|---|---|
| 13423427–30 | `765.roms_r-benchmark2-200B` | all 4 | ~38 min |
| 13423431–34 | `765.roms_r-benchmark2-430B` | all 4 | ~73 min |
| 13423806 | `881.neutron_s-xsbench_history_nuclide-21B` | `pythia_with_xpt_uncore_p` only | ~42 min |

All nine aborted in `print_deadlock` with the **same signature** — a memory load
issued off-chip but never completed:

```
DEADLOCK! CPU 0 instr_id: 237622855 translated: 2 fetched: 2 scheduled: 1 executed: 0 \
          is_memory: 1 event: 145580469 current: 146580469
```

`is_memory:1 fetched:2 executed:0` = the load's request went to memory but its
completion never came back, so the ROB head never retires. The Load Queue dump that
follows is full of younger loads stuck at `address: 0 fetched: 0` (blocked behind the
head), then the LLC MSHR dump. (`current − event = 1,000,000` = the watchdog window.)

### Diagnosis so far

- **Not a regression from the cosmetic refactors.** 588/600 runs completed cleanly
  (exit 0) across all four configs on the other 147 traces — consistent with the
  earlier byte-identical verification of the clang-format and knob-framework changes.
  A cosmetic bug would fail broadly or by *config*; this fails by **trace**.
- **Concentrated on the most memory-bound traces** (roms HPC simpoints fully; one
  neutron/xsbench variant). The symptom (a dropped off-chip completion) points at the
  **memory / DDRP return path under heavy off-chip traffic**, *or* a pre-existing
  ChampSim deadlock that these traces happen to hit.
- **Blame is not yet assignable** because *every* experiment in this batch uses the new
  uncore + DDRP path — there is no core-mode or DDRP-off arm to compare against.

### Reconfirmed in a second batch (2026-06-26)

Batch `20260625T121924Z_spec26_xpt_uncore_so` (same uncore+DDRP configs but at
`ddrp_req_latency=0`) deadlocked on **exactly** `765.roms_r-benchmark2-200B` and
`-430B` again — both variants, same `print_deadlock` signature. So the hang is
**independent of `ddrp_req_latency`** (0 / 1 / 8 all deadlock); the DDRP *latency*
is not the trigger, which lets the bisect below drop latency as a variable. The
`881.neutron_s-...xsbench...-21B` case did **not** recur, i.e. it is intermittent —
the two roms simpoints are the reliable repro. (Everything else completed cleanly,
including the slow `853.ns3_s-tcp_validation-202B` runs at ~9–10 h.)

### Where to start next (bisect plan)

Reproduce the fastest case — `765.roms_r-benchmark2-200B`, `xpt_uncore_o`
(~38 min). Trace:
`/mnt/galactica/rahbera/tracezoo/champsim/version2/spec26/765.roms_r-benchmark2-200B.champsim2.zst`

Baseline failing command (Arm C):

```
bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch \
  --warmup_instructions=100000000 --simulation_instructions=500000000 \
  --llc_replacement_type=ship --config=config/nopref.ini \
  --num_rob_partitions=3 --rob_partition_size=64,128,320 \
  --rob_frontal_partition_ids=0 --rob_dorsal_partition_ids=2 \
  --config=config/ocp_xpt.ini --offchip_pred_location=uncore \
  --ocp_xpt_use_physical_address=true \
  --config=config/hermes_base.ini --ddrp_req_latency=1 \
  -traces <trace>
```

Run these arms on the same trace and compare who deadlocks:

- **Arm A — plain BASE** (drop `ocp_xpt.ini` + `hermes_base.ini`, no predictor, no DDRP).
  If it still deadlocks → **pre-existing trace/core deadlock**, unrelated to our work; close.
- **Arm B — uncore prediction, DDRP off** (keep `ocp_xpt.ini` +
  `--offchip_pred_location=uncore --ocp_xpt_use_physical_address=true`, but **drop
  `hermes_base.ini`** so `enable_ddrp` stays default false). Isolates the uncore
  *prediction* hooks from the DDRP *action*.
- **Arm C — the failing config** (above): confirms the repro.
- **Arm D — pre-refactor binary.** Rebuild at the commit *before* the clang-format /
  knob work (e.g. `fefdc11` or `8bbf0b3`, which already contain the uncore-DDRP path) and
  run Arm C. Rules the recent refactors in or out.

Interpretation: A deadlocks ⇒ pre-existing; A passes & B deadlocks ⇒ uncore-prediction
path; A,B pass & C deadlocks ⇒ DDRP-action path; D also deadlocks ⇒ not introduced by
the refactors. (v2 buffered trace reading is an unlikely sole cause — 588 other v2
traces passed — but if A–D are inconclusive, try a v1 build of the same trace.)

### Code to look at

- `print_deadlock()` / watchdog: [../src/main.cc:1034](../src/main.cc#L1034).
- Uncore predict/train hooks at the LLC: `CACHE::handle_read()` in
  [../src/cache.cc](../src/cache.cc) (the `IS_LLC` + `offchip_pred_location==uncore` path).
- Uncore DDRP / speculative-direct-DRAM path and its completion/return: introduced in
  commits `fefdc11` (uncore DDRP) and `8afa015` (XPT uncore predictor); DDRP buffer in
  [../src/dram_controller.cc](../src/dram_controller.cc).
- Load completion / LQ release: [../src/ooo_cpu.cc](../src/ooo_cpu.cc).
- [../src/offchip_pred.cc](../src/offchip_pred.cc) — predictor ownership/aliasing in
  `initialize_offchip_predictor` (one shared LLC-owned instance in uncore mode).
