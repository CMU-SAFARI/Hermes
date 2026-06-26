---
name: regression
description: Use when the user wants to check whether a Hermes change regresses core-mode off-chip-predictor behavior, run the regression suite, or compare a build against a baseline. Covers "run a regression", "did my change regress", "check core-mode equivalence (IPC / precision / recall)", "regression-test this refactor", "compare vs HEAD/main". Drives champsim-infra/regression locally (no cluster).
---

# Hermes Regression

Verify a change keeps Hermes **bit-for-bit identical in `core` mode**. ChampSim is
deterministic, so identical binary+config+trace must give identical stats. While
the off-chip predictor stays in `core` mode (i.e. through the uncore-relocation
work, until the uncore path is intentionally enabled) these must not move:

- `Core_0_cumulative_IPC`
- `Core_0_offchip_pred_precision`
- `Core_0_offchip_pred_recall`

Any change in any tracked metric for any (trace, exp) pair is a **regression**.
This skill **drives** the existing harness — it does not reimplement rollup/diff:

```
HARNESS=/home/rahbera/thesis/champsim-infra/regression   # run_regression.py, compare_runs.py
HELPER=.claude/skills/regression/compare_change.sh        # Mode 1 two-build dance
```

Runs are **fully local** (no SSH) but heavy (builds + multiple ChampSim runs) —
confirm before launching and offer the quick single-trace variant.

## Always do first: orient

1. Resolve the repo: `git -C <cwd> rev-parse --show-toplevel` (expect Hermes).
2. Pick the mode from intent:
   - "did my change regress" / "compare vs HEAD/main" → **Mode 1** (default).
   - "run the regression suite" / "diff vs my last run" → **Mode 2**.
3. Parallelism is mandatory. The helper computes `max(1, nproc-2)`; for Mode 2
   compute it yourself and pass `--local-parallel $PAR` to every run.

## Mode 1 — compare my change (default)

Two-build comparison of a baseline ref (default `HEAD`) vs the working tree, in a
throwaway `git worktree` so the working copy is never mutated. Run the helper:

```
.claude/skills/regression/compare_change.sh \
    [--ref HEAD] [--out /home/rahbera/thesis/runs] \
    [--tlist F] [--exp F] [--mfile F] \
    [--build "./build_champsim.sh glc multi multi multi multi 1 1 0"] \
    [--parallel N]      # default max(1, nproc-2)
```

It (1) refuses if the working tree equals `--ref` (nothing to compare — ask for
another ref, e.g. `HEAD~1` / `main`); (2) builds the working tree → candidate;
(3) builds `--ref` in a worktree → baseline; (4,5) runs `run_regression.py` on
each binary into `--out`; (6) `compare_runs.py <baseline> <candidate>` →
**exit 0 = PASS, non-zero = REGRESSION**; (7) removes the worktree (always).

Report the compare table + verdict: **PASS — no change** or
**REGRESSION — N (trace,exp) pair(s) changed** (list the changed rows).

Quick variant: point `--tlist` at a one-trace list (e.g. just `462.libquantum`)
for a fast sanity check instead of the full 5-trace suite.

## Mode 2 — run the suite / diff vs last run

Thin driver of the harness's native auto-diff (vs the most recent previous run in
`OUTPUT_DIR`). Build the current tree (or use a given `--exe`), then:

```
PAR=$(nproc); PAR=$(( PAR>2 ? PAR-2 : 1 ))
cd /home/rahbera/thesis/champsim-infra/regression
python3.12 run_regression.py /home/rahbera/thesis/runs \
    --exe /home/rahbera/thesis/Hermes/bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch \
    --tlist suites/test_suite.tlist.yml \
    --exp   suites/regression.exp.yml \
    --mfile suites/regression.mfile.yml \
    --local-parallel "$PAR" [--label <tag>]
```

Report its built-in compare output.

## Defaults & notes

- Build: `./build_champsim.sh glc multi multi multi multi 1 1 0`
  → `glc-perceptron-no-multi-multi-multi-multi-1core-1ch`.
- `OUTPUT_DIR`: `/home/rahbera/thesis/runs` (outside the repo).
- Suites: harness `suites/{test_suite.tlist,regression.exp,regression.mfile}.yml`
  (core_popet, 5 SPEC traces, ipc/precision/recall). Full suite by default.
- Baseline ref `HEAD`; comparison exact (add `compare_runs.py --tol` only if asked).
- Parallelism: always `max(1, nproc-2)`.
- Python: the harness shebangs `python3.12` (this host's `python3` is 3.8); use it.
- A failed/deadlocked trace is `Filter=0` in rollup — report it as inconclusive,
  not a pass. The harness snapshots each `--exe`, so a rebuild mid-run is safe.
