#!/usr/bin/env bash
# compare_change.sh — Hermes core-mode regression (skill `regression`, Mode 1).
# Compare the working tree against a baseline git ref by building both and diffing
# the regression suite. The baseline is built in a throwaway `git worktree`, so
# the working copy is never mutated.
#
# Usage: compare_change.sh [--ref HEAD] [--out DIR] [--tlist F] [--exp F]
#                          [--mfile F] [--build "CMD"] [--parallel N]
# Exit:  0 = PASS (identical core-mode metrics), 1 = REGRESSION, 2 = setup error.
set -euo pipefail

REF=HEAD
OUT=/home/rahbera/thesis/runs
BUILD="./build_champsim.sh glc multi multi multi multi 1 1 0"
HARNESS=/home/rahbera/thesis/champsim-infra/regression
TLIST="$HARNESS/suites/test_suite.tlist.yml"
EXP="$HARNESS/suites/regression.exp.yml"
MFILE="$HARNESS/suites/regression.mfile.yml"
PY=python3.12
PAR=

while [ $# -gt 0 ]; do
  case "$1" in
    --ref) REF=$2; shift 2 ;;
    --out) OUT=$2; shift 2 ;;
    --build) BUILD=$2; shift 2 ;;
    --tlist) TLIST=$2; shift 2 ;;
    --exp) EXP=$2; shift 2 ;;
    --mfile) MFILE=$2; shift 2 ;;
    --parallel) PAR=$2; shift 2 ;;
    -h | --help)
      sed -n '2,9p' "$0"
      exit 0
      ;;
    *)
      echo "compare_change.sh: unknown arg '$1'" >&2
      exit 2
      ;;
  esac
done

# Parallelism: default max(1, nproc - 2) — leave 2 threads free.
if [ -z "$PAR" ]; then
  n=$(nproc)
  PAR=$((n > 2 ? n - 2 : 1))
fi

REPO=$(git rev-parse --show-toplevel)
cd "$REPO"
BIN="bin/glc-perceptron-no-multi-multi-multi-multi-1core-1ch" # produced by $BUILD

[ -d "$OUT" ] || {
  echo "compare_change.sh: OUTPUT_DIR not found: $OUT" >&2
  exit 2
}
REF_SHA=$(git rev-parse --verify "$REF^{commit}" 2>/dev/null) || {
  echo "compare_change.sh: bad --ref '$REF'" >&2
  exit 2
}

# 1. Sanity: there must be a difference between the ref and the working tree.
if git diff --quiet "$REF_SHA"; then
  echo "compare_change.sh: working tree is identical to $REF — nothing to compare." >&2
  echo "  pass a different --ref (e.g. HEAD~1, main, <sha>)." >&2
  exit 2
fi

WT="${TMPDIR:-/tmp}/hermes-baseline-$$"
trap 'git worktree remove --force "$WT" 2>/dev/null || true; git worktree prune 2>/dev/null || true' EXIT

build_in() { # $1=dir  $2=label
  local log="${TMPDIR:-/tmp}/regress-build-$2-$$.log"
  echo "### build $2 ($1)" >&2
  if ! (cd "$1" && eval "$BUILD") >"$log" 2>&1; then
    echo "compare_change.sh: $2 build FAILED:" >&2
    tail -15 "$log" >&2
    exit 2
  fi
  [ -x "$1/$BIN" ] || {
    echo "compare_change.sh: $2 build produced no $BIN" >&2
    exit 2
  }
}

# 2. candidate (working tree), then 3. baseline (worktree checked out at REF)
build_in "$REPO" candidate
git worktree add --detach "$WT" "$REF_SHA" >&2
# libbf is a gitignored, prebuilt vendored dependency (Makefile: -I./libbf/ and
# libbf/build/lib/libbf.a); a fresh worktree checkout lacks it, so link the main
# repo's copy in before building the baseline.
if [ -e "$REPO/libbf" ] && [ ! -e "$WT/libbf" ]; then
  ln -s "$REPO/libbf" "$WT/libbf"
fi
build_in "$WT" baseline

run() { # $1=exe  $2=label  -> prints the harness run dir on stdout
  local log
  log=$("$PY" "$HARNESS/run_regression.py" "$OUT" --exe "$1" \
    --tlist "$TLIST" --exp "$EXP" --mfile "$MFILE" \
    --local-parallel "$PAR" --label "$2" 2>&1) || {
    echo "$log" >&2
    echo "compare_change.sh: run '$2' FAILED" >&2
    exit 2
  }
  echo "$log" >&2 # surface the harness output (summary + its own auto-diff)
  echo "$log" | sed -n 's/^=== done: \(.*\) ===$/\1/p' | tail -1
}

# 4 + 5. run each binary through the harness (snapshots the exe per run)
echo "### regression: $REF (baseline) vs working tree (candidate), parallel=$PAR" >&2
BASE_DIR=$(run "$WT/$BIN" "baseline-${REF_SHA:0:8}")
CAND_DIR=$(run "$REPO/$BIN" "candidate")

# 6. authoritative diff of the two known run dirs
echo "### compare (exit 0 = identical)" >&2
if "$PY" "$HARNESS/compare_runs.py" "$BASE_DIR" "$CAND_DIR"; then
  echo ">>> PASS: core-mode metrics identical ($REF vs working tree)."
  exit 0
else
  echo ">>> REGRESSION: core-mode metrics changed vs $REF (see table above)."
  exit 1
fi
