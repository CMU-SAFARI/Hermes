# Design: `regression` skill — core-mode equivalence gate for Hermes

- **Date:** 2026-06-26
- **Status:** Design (approved); pending implementation
- **Type:** Hermes-local Claude Code skill that drives the existing
  `champsim-infra/regression` harness.

## Context & goal

The off-chip predictor is being relocated from the core to the uncore in stages.
Throughout that work the predictor must stay **bit-for-bit identical in `core`
mode** — ChampSim is deterministic, so identical binary+config+trace must produce
identical stats. `champsim-infra/regression/run_regression.py` already automates
"run the suite → roll up → diff," and `compare_runs.py` diffs two runs. What is
missing is an ergonomic, repeatable way to answer the question that actually comes
up during development: **"does my uncommitted change regress core-mode behavior?"**
Today that requires the manual two-build dance (build HEAD, build working tree,
run both, diff). This skill makes that one invocation, and also exposes the
harness's native "run the suite / diff vs last run" flow.

This skill **drives** the existing pipeline; it does not reimplement rollup,
diffing, or jobfile generation.

## The invariant it guards

While `offchip_pred_location=core`, these metrics (the harness's default
`regression.mfile.yml`) must not change:

- `Core_0_cumulative_IPC`
- `Core_0_offchip_pred_precision`
- `Core_0_offchip_pred_recall`

A change in any tracked metric for any (trace, exp) pair is a **regression**.
Comparison is **exact** by default (the determinism premise); relative tolerance
(`compare_runs.py --tol`) is only used when the user explicitly asks.

## Placement & structure

Hermes-local, beside `git-commit`:

```
Hermes/.claude/skills/regression/
  SKILL.md             # orientation + the two modes + defaults + reporting
  compare_change.sh    # bundled helper for Mode 1 (the worktree two-build dance)
```

`SKILL.md` is the decision/orchestration layer (which mode, defaults, how to read
the result). `compare_change.sh` is a single well-bounded unit that performs the
two-build comparison atomically and cleans up after itself — mirroring how
`git-commit` bundles `format.sh`.

## Two modes, selected by intent

The skill's "orient" step picks a mode from what the user asked:

- **Mode 1 — compare my change (DEFAULT).** Two-build comparison: a baseline ref
  (default `HEAD`) vs the working tree. Answers "did my uncommitted change
  regress core mode?"
- **Mode 2 — run the suite / diff vs last run.** Thin driver of
  `run_regression.py` using its native auto-diff against the most recent previous
  run in `OUTPUT_DIR`. For "run the regression suite" / "compare vs my last run."

## Mode 1 mechanics — `compare_change.sh`

Approach **A** (temporary `git worktree`): build the baseline ref in an isolated
worktree so the working copy is never mutated.

```
compare_change.sh [--ref HEAD] [--out /home/rahbera/thesis/runs]
                  [--tlist F...] [--exp F...] [--mfile F...]
                  [--build "./build_champsim.sh glc multi multi multi multi 1 1 0"]
                  [--parallel N]      # default: max(1, nproc - 2)
```

Flow (a `trap` guarantees worktree removal on any exit):

1. **Sanity.** If `--ref` resolves to the same tree as the working copy (e.g. no
   uncommitted changes vs `HEAD`), stop and ask which ref to compare against
   (`HEAD~1`, `main`, a sha) — comparing a tree against itself is meaningless.
2. **Build candidate** in the working tree → `candidate_exe`.
3. **Build baseline:** `git worktree add <tmp> <ref>`; run the build command in
   `<tmp>` → `baseline_exe`. (`run_regression.py` snapshots each `--exe` into its
   run dir, so a later rebuild cannot perturb an already-recorded run.)
4. `run_regression.py <out> --exe <baseline_exe> --tlist … --exp … --mfile …
   --local-parallel <PAR> --label baseline-<ref>` → `baseline_dir`.
5. `run_regression.py <out> --exe <candidate_exe> --tlist … --exp … --mfile …
   --local-parallel <PAR> --label candidate` → `candidate_dir`.
6. `compare_runs.py <baseline_dir> <candidate_dir>` → **exit 0 = PASS**
   (identical), non-zero = regression. Explicit two-dir compare, so the result
   never depends on "most recent previous run" guessing.
7. `git worktree remove --force <tmp>` (+ `git worktree prune`).

## Mode 2 mechanics — thin driver

Build the current tree (or accept a user-supplied `--exe`), then:

```
run_regression.py <out> --exe <exe> --tlist … --exp … --mfile …
                  --local-parallel <PAR> [--label …]
```

Report `run_regression.py`'s built-in auto-diff against the most recent previous
run in `OUTPUT_DIR`.

## Parallelism rule (both modes)

**Always run the suite in parallel on the local node.** Before generating jobs,
detect hardware threads and target **N − 2** concurrent ChampSim runs:

```
PAR=$(nproc); PAR=$(( PAR > 2 ? PAR - 2 : 1 ))
```

Pass `--local-parallel $PAR` to every `run_regression.py` call (it forwards the
value to `create_jobfile.py`'s local `MAX_PARALLEL` throttle). On this 12-thread
host that is **10** concurrent runs, so the 5-trace suite completes in roughly one
trace's wall-time instead of five. Leaving 2 threads free keeps the machine
responsive and avoids oversubscription.

## Defaults

| Knob | Default |
|------|---------|
| Mode | Mode 1 (compare my change) |
| Build command | `./build_champsim.sh glc multi multi multi multi 1 1 0` → `glc-perceptron-no-multi-multi-multi-multi-1core-1ch` |
| `OUTPUT_DIR` | `/home/rahbera/thesis/runs` (outside the repo) |
| Suites | harness `suites/{test_suite.tlist,regression.exp,regression.mfile}.yml` (core_popet, 5 SPEC traces, ipc/precision/recall) |
| Scope | **full 5-trace suite** (quick single-trace tlist offered as an override) |
| Baseline ref | `HEAD` |
| Parallelism | `max(1, nproc − 2)` |
| Comparison | exact (no `--tol`) |

## Reporting

Surface the `compare_runs.py` table and a one-line verdict:

- **PASS — no change vs baseline**, or
- **REGRESSION — N (trace,exp) pair(s) changed**, listing the changed rows
  (metric, baseline value, candidate value).

## Edge cases

- **Nothing to compare** (working tree == ref): prompt for an alternative ref.
- **Build failure** (either binary): report which build failed with its log tail;
  do not run the suite; still clean up the worktree.
- **A run fails / a trace deadlocks:** `rollup.py` marks the trace `Filter=0`; the
  skill reports filtered (trace,exp) rows as inconclusive rather than as a pass.
- **Interrupted run:** the `trap` removes the worktree; the working tree is never
  left mutated (the worktree approach touches only `<tmp>`).

## Cost & permissions

Runs **fully local** (no SSH, no network). Mode 1 = 2 builds + 2× the suite. With
N − 2 parallelism the full suite at 10M warmup / 30M sim is on the order of
~10–20 min wall-clock; confirm before launching, and offer the quick single-trace
variant for fast sanity checks. `OUTPUT_DIR` lives outside the repo so dumps are
never committed.

## Out of scope (future)

- Promoting the two-build comparison into `run_regression.py` itself as a
  `--baseline-ref` flag (then the skill's Mode 1 would shrink to a single harness
  call). Kept in the skill layer for now so the infra stays untouched.
- Making the skill global / multi-sim (like `cluster-run`). Hermes-local for now.
- Tolerance-based comparison, uncore-mode suites — both already reachable by
  passing the harness's existing flags/suites; not defaulted.

## Verification (how we know the skill works)

1. **No-op change → PASS.** Run Mode 1 with no working-tree change vs `HEAD~1`
   where `HEAD` is a known behavior-preserving commit (e.g. the clang-format or
   the perc-helper refactor): expect `PASS — no change`.
2. **Injected change → REGRESSION.** Temporarily perturb a core-mode stat (or
   point `--ref` at a commit known to differ) and confirm the skill reports the
   changed (trace,exp) rows and a non-zero verdict.
3. **Parallelism honored.** Confirm the generated jobfile runs `nproc − 2`
   concurrent ChampSim processes (e.g. 10 on this host).
4. **Working tree untouched.** After a Mode 1 run (including a forced mid-run
   abort), `git status` shows no unexpected changes and no leftover worktree.
