#!/usr/bin/env bash
# format.sh — enforce the repo's .clang-format on the files about to be committed.
#
# Used by the git-commit skill as a pre-commit gate so every commit lands already
# formatted to the codebase norm. Repo-agnostic: it keys off a .clang-format at
# the repo root and no-ops cleanly when there isn't one.
#
# Usage:
#   format.sh             check changed-vs-HEAD C++ files; exit 1 if any need formatting
#   format.sh --fix       reformat those files in place (clang-format -i)
#   format.sh --all       operate on ALL tracked C++ files instead of just changed
#   format.sh --fix --all
#
# "C++ files" = *.cc *.cpp *.cxx *.h *.hpp *.hh plus the ChampSim source files that
# use custom extensions (.l1i_pref .l1d_pref .l2c_pref .llc_pref .bpred .llc_repl);
# clang-format treats them as C++ via the .clang-format `Language: Cpp`.
#
# Exit codes: 0 = clean / fixed / skipped, 1 = files need formatting (check mode),
#             2 = bad usage, 3 = .clang-format present but clang-format missing.

set -euo pipefail

FIX=0
ALL=0
for arg in "$@"; do
  case "$arg" in
    --fix) FIX=1 ;;
    --all) ALL=1 ;;
    -h | --help)
      sed -n '2,20p' "$0"
      exit 0
      ;;
    *)
      echo "format.sh: unknown argument '$arg'" >&2
      exit 2
      ;;
  esac
done

ROOT=$(git rev-parse --show-toplevel)
cd "$ROOT"

if [ ! -f .clang-format ]; then
  echo "[format] no .clang-format at repo root; nothing to enforce."
  exit 0
fi

CF=$(command -v clang-format || true)
if [ -z "$CF" ]; then
  echo "[format] ERROR: .clang-format exists but clang-format is not installed." >&2
  echo "[format]        install clang-format (e.g. apt-get install clang-format)." >&2
  exit 3
fi

GLOBS=('*.cc' '*.cpp' '*.cxx' '*.h' '*.hpp' '*.hh'
  '*.l1i_pref' '*.l1d_pref' '*.l2c_pref' '*.llc_pref' '*.bpred' '*.llc_repl')

if [ "$ALL" -eq 1 ]; then
  mapfile -t CANDIDATES < <(git ls-files "${GLOBS[@]}")
else
  # Everything that differs from HEAD: staged, unstaged, and new untracked files.
  mapfile -t CANDIDATES < <( {
    git diff --name-only HEAD -- "${GLOBS[@]}" 2>/dev/null
    git diff --cached --name-only -- "${GLOBS[@]}" 2>/dev/null
    git ls-files --others --exclude-standard -- "${GLOBS[@]}" 2>/dev/null
  } | sort -u)
fi

# A path can appear for a deleted/renamed file; keep only ones that exist now.
FILES=()
for f in "${CANDIDATES[@]:-}"; do
  [ -n "$f" ] && [ -f "$f" ] && FILES+=("$f")
done

if [ "${#FILES[@]}" -eq 0 ]; then
  echo "[format] no C++ files to format."
  exit 0
fi

if [ "$FIX" -eq 1 ]; then
  # clang-format is not always idempotent (e.g. AlignTrailingComments around a
  # commented-out member can take two passes to settle). Re-run to a fixed point
  # so the committed file is stable and a single-pass check will pass.
  MAX=5
  for ((pass = 1; pass <= MAX; pass++)); do
    "$CF" -i --style=file "${FILES[@]}"
    if "$CF" --style=file --dry-run --Werror "${FILES[@]}" >/dev/null 2>&1; then
      echo "[format] reformatted ${#FILES[@]} file(s); fixed point in $pass pass(es)."
      exit 0
    fi
  done
  echo "[format] WARNING: no clang-format fixed point after $MAX passes; stubborn file(s):" >&2
  for f in "${FILES[@]}"; do
    if ! "$CF" --style=file --dry-run --Werror "$f" >/dev/null 2>&1; then
      echo "  $f" >&2
    fi
  done
  echo "[format] consider wrapping the unstable region in // clang-format off/on." >&2
  exit 4
fi

# Check mode: --dry-run --Werror returns non-zero if any file needs formatting.
if "$CF" --style=file --dry-run --Werror "${FILES[@]}" >/dev/null 2>&1; then
  echo "[format] ${#FILES[@]} C++ file(s) already conform to .clang-format."
  exit 0
fi

echo "[format] the following file(s) are NOT formatted (run '$0 --fix'):" >&2
for f in "${FILES[@]}"; do
  if ! "$CF" --style=file --dry-run --Werror "$f" >/dev/null 2>&1; then
    echo "  $f" >&2
  fi
done
exit 1
