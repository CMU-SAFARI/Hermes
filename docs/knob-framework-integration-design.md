# Design: Integrate Athena's X-macro Knob Framework into Hermes

**Date:** 2026-06-22
**Status:** Approved — pre-implementation
**Reference framework:** `/home/rahbera/thesis/Athena` (`inc/knobs.h`, `inc/knobs.def`, `src/knobs.cc`)

## Problem

Hermes's knob system is spread across three hand-maintained piles:
- `namespace knob { extern … }` blocks duplicated in **45 files**.
- a **1352-line** `src/knobs.cc` with a hand-written `parse_knobs` `else if` chain.
- a **351-line** `print_knobs()` in `src/main.cc` that dumps every knob by hand.

Adding one knob means editing all three. Athena replaces this with a single
source of truth — `knobs.def` — expanded by X-macros into the externs, the
definitions, the parse chain, and (here) the dump. Consumers only
`#include "knobs.h"`.

## Decisions (locked with the user)

- **Faithful & minimal knob set.** The new `knobs.def` contains exactly Hermes's
  current knobs (their names/types/defaults), plus the 7 recent additions Athena
  lacks (`offchip_pred_location`, `ocp_xpt_num_entries`, `ocp_xpt_assoc`,
  `ocp_xpt_offchip_threshold`, `ocp_xpt_hash_type`, `ocp_xpt_use_physical_address`,
  `enable_dynamic_packet_priority`). The 93 Athena-only knobs are **excluded**.
- **Macro-generate the dump.** `print_knobs()` becomes macro-driven for scalar
  knobs (eliminating most of the 351 lines). It is a **hybrid**: the X-macro emits
  the scalar-knob lines, and manual lines are retained for the non-knob constants
  it also prints (`NUM_CPUS`, cache/DRAM params, `ship.*`) and the complex knobs.
  The dump becomes flat def-order (loses the current section grouping — accepted).
- **One commit** for the whole integration, prepared via the git-commit skill
  (which now runs the clang-format gate).

## Components

1. **`inc/knobs.def`** (new) — one `DEF_KNOB(opt, name, type, parser, defval)` per
   **simple scalar** knob (~290). Built from Hermes's current knobs: variable name
   and default from the `type name = default;` block in today's `knobs.cc`; parser
   from the `knob::name = atoX(value)` line (`atoi→uint32`/`int32`, `atol→uint64`,
   `!strcmp(...,"true")→bool`, `string(...)→string`, `atof→float`,
   `strtod→double`). Shared knobs cross-checked against Athena's def (defaults
   already shown to match, modulo `string("x")` vs `"x"`).

2. **`inc/knobs.h`** (rewrite, Athena-style) — inside `namespace knob`:
   `#define DEF_KNOB(...) extern type name;` + `#include "knobs.def"`, then manual
   `extern`s for the **complex/derived** knobs (below). Plus prototypes:
   `parse_args` / `parse_config` / `parse_knobs` / `handler`, the `get_*` helpers,
   and `void print_knobs();`. This single header replaces all 45 extern blocks.

3. **`src/knobs.cc`** (rewrite, Athena-style) — macro-expanded definitions
   (`type name = defval;`); macro-expanded `parse_knobs` chain
   (`else if (MATCH("", #opt)) { knob::name = get_##parser(value); }`); the `get_*`
   helpers; **Hermes's complex-knob handlers ported verbatim**; and the hybrid
   `print_knobs()`. Unknown-knob behavior unchanged (print "unable to parse",
   return 0 — identical in both repos today).

4. **`src/main.cc`** — remove the `print_knobs()` definition (moves to `knobs.cc`);
   keep the call site (~line 1681).

5. **45 consumer files** — delete the `namespace knob { extern … }` blocks; ensure
   `#include "knobs.h"`.

### Complex / derived knobs (hand-written, NOT macro; ported from Hermes)

- `l1d_prefetcher_types`, `l2c_prefetcher_types`, `llc_prefetcher_types` (vector,
  `push_back` per value).
- `rob_partition_size` (+ derived `rob_partition_boundaries`),
  `rob_frontal_partition_ids`, `rob_dorsal_partition_ids` (vectors with
  `assert(... == num_rob_partitions)`, `assert(len == ROB_SIZE)`, boundary math,
  range asserts).
- scooby vector knobs and the derived `scooby_max_actions = scooby_actions.size()`.
- Any remaining knob whose parse has side effects — enumerated during
  implementation and kept manual.

## Rollout (incremental; final history = one commit)

Extern declarations are idempotent, so the framework lands first and *validates*
type consistency against the still-present old blocks.

1. **Phase 1** — author `knobs.def`/`.h`/`.cc`, move `print_knobs`. Build with the
   45 old extern blocks still in place (duplicate `extern`s are legal; a type
   mismatch becomes a compile error — a free safety net). Run behavioral identity.
2. **Phase 2** — strip the extern blocks from the 45 files (mechanical, batched;
   parallelizable), building between batches.
3. **Phase 3** — final build + behavioral identity + knob-coverage diff. Squash to
   one commit.

## Verification gate (same method as the v2 / clang-format work)

- **Build:** the configs that compile every custom-extension file link cleanly.
- **Behavioral identity:** the refactored binary is byte-identical to the
  pre-refactor binary on XPT+Hermes+DDRP+SHiP, Scooby/Pythia, and SPP_dev2
  (462.libquantum, 5M/15M) — a structural refactor with identical knob values must
  produce identical sim output. The startup **dump block is excluded** from the
  diff (now flat def-order; cosmetic). Knob values are verified separately:
- **Knob-coverage diff:** assert the set of knob names parsed today (346) is a
  subset of the new def + complex set, each with a matching default. No config may
  silently stop parsing.

## Out of scope

The 93 Athena-only knobs; any new knob; any logic change; the dump's section
grouping.

## Risks

- Per-knob extraction error (wrong type/default/parser) → caught by build +
  behavioral identity + the knob-coverage diff; non-exercised knobs cross-checked
  against Athena.
- Complex/derived knobs come from **Hermes**, not Athena (the forks' vector logic
  may differ).
- A consumer file that relied on the extern block without including `knobs.h` must
  gain the include when its block is stripped (per-file in Phase 2).

## Success criteria

- `knobs.def` is the single source of truth; adding a knob touches only it.
- All 45 extern blocks gone; consumers `#include "knobs.h"` only.
- Build links; behavioral identity holds on all three configs; knob-coverage diff
  is empty.
