# Design: Codebase-wide clang-format

**Date:** 2026-06-22
**Status:** Approved — executing
**Scope:** All tracked C++ source in the Hermes repo, including custom-extension files.

## Goal

Apply one canonical clang-format style across the whole codebase:
- 2-space indentation, never tabs.
- All `if/else/for/while` bodies braced (safety).
- 80-column limit (lines reflow so nothing runs past 80 — readable in two
  side-by-side editor tabs).

Today the code is inconsistent (e.g. `cache.cc` mixes 176 tab-lines with 2049
space-lines). A normalize pass fixes this.

## The `.clang-format` (repo root)

```yaml
BasedOnStyle: LLVM
Language: Cpp
IndentWidth: 2
TabWidth: 2
UseTab: Never
ColumnLimit: 80
ContinuationIndentWidth: 4
AccessModifierOffset: -2
NamespaceIndentation: None
BreakBeforeBraces: Linux       # Allman funcs/ns/class; attached + cuddled control
InsertBraces: true            # every if/else/for/while braced
FixNamespaceComments: true
PointerAlignment: Right         # '*'/'&' bind to the variable name
ReferenceAlignment: Right
DerivePointerAlignment: false
AlignConsecutiveAssignments: true   # align '=' in back-to-back assignments
AlignConsecutiveDeclarations: true  # align variable names in consecutive decls
AlignConsecutiveMacros: true
AlignTrailingComments: true
AlignOperands: Align
AlignAfterOpenBracket: Align
AlignEscapedNewlines: Left
ReflowComments: true            # comments obey the 80-col limit
SpacesBeforeTrailingComments: 2
AllowShortFunctionsOnASingleLine: Inline
AllowShortBlocksOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
MaxEmptyLinesToKeep: 2
KeepEmptyLinesAtTheStartOfBlocks: false
SortIncludes: false           # not requested; reordering can break implicit deps
Cpp11BracedListStyle: true
SpaceBeforeParens: ControlStatements
```

Reference: based on the alignment/spacing options in
[litz-lab/scarab's .clang-format](https://github.com/litz-lab/scarab/blob/main/.clang-format),
with two deliberate overrides: `PointerAlignment: Right` (scarab uses `Left`; we
want `*` on the name) and `BreakBeforeBraces: Linux` (scarab attaches all braces;
we keep Allman for functions/namespaces/classes). `SortIncludes: false` kept off
(not requested; reordering includes is the most common way formatting silently
breaks a build).

## Scope & file discovery

Format all **tracked** files matching:
`*.cc *.h *.l1i_pref *.l1d_pref *.l2c_pref *.llc_pref *.bpred *.llc_repl`
(~161 files). clang-format treats the custom-extension files as C++ via
`--assume-filename=x.cc`.

Excluded automatically (untracked / gitignored, so `git ls-files` never lists
them): the generated copies `prefetcher/*_prefetcher.cc`,
`branch/branch_predictor.cc`, `replacement/llc_replacement.cc`, `inc/defs.h`
(regenerated from the `.l1d_pref`/`.bpred`/`.llc_repl` source at build), and the
vendored `libbf/` library.

## Safety net (verification gate)

Mirrors the v1 trace-neutrality method already used in this repo:

1. **Build gate.** Build the set of configs needed to compile every custom-extension
   source (the only files whose compilation depends on build args; all `prefetcher/*.cc`
   etc. are glob-compiled by every build). Every build must succeed.
2. **Behavioral-identity gate.** For representative configs, run the formatted binary
   vs the pre-format binary on a sample trace and diff sim stats (wall-clock stripped).
   Result must be **byte-identical** — formatting is whitespace + braces only, so any
   difference means clang-format changed semantics (e.g. an `InsertBraces` macro edge
   case) and is a bug to fix, not accept.
3. **Fragile-file fallback.** Spot-check `spp_dev2.{cc,h}`, `spp_dev2_helper.{cc,h}`,
   and macro-heavy headers. Anything that fails the build or is visibly mangled gets
   `// clang-format off` / `// clang-format on` guards or a `.clang-format-ignore`
   entry, then re-verified. (clang-format 18 already handled `spp_dev2_helper.h`
   cleanly in a dry run, so this is a backstop.)

## Rollout & git hygiene

- Dedicated branch off `rbdev` (the diff touches ~161 files; isolate for review).
- Commits: (1) add `.clang-format`; (2) "Apply clang-format across the codebase";
  (3) append the format commit's SHA to `.git-blame-ignore-revs` and set
  `git config blame.ignoreRevsFile .git-blame-ignore-revs` so `git blame` skips it.
- During execution, format directory-by-directory with a build check between groups
  so any breakage is localized; final history is the single format commit.

## Out of scope

No logic changes, no include reordering, no renames, no comment rewrapping. Whitespace
and braces only — enforced by the behavioral-identity gate.

## Success criteria

- All targeted files formatted; `git diff` shows only whitespace/brace/format changes.
- Every required build config compiles.
- Behavioral-identity holds: formatted vs pre-format sim stats identical on ≥1 trace
  per major config.
- `spp_dev*` and macro-heavy files build and are not visibly mangled.
