# AI Assistant Rules for Hermes

Please follow the repository AI rules below when working in Hermes.

## Project Overview

Hermes is a ChampSim-based out-of-order CPU simulator for **off-chip load
prediction** research. The original Hermes mechanism (Bera et al., MICRO 2022)
predicts which loads will miss every on-chip cache using a lightweight
perceptron-based predictor (POPET) and speculatively fetches their data straight
from DRAM in parallel with the cache lookups, removing on-chip cache latency from
the critical path. See [README.md](../README.md) for the upstream description and
the build / trace / experiment workflow.

This fork extends that baseline to study **which** off-chip predictor to use and
**where** it should live:

- **Predictor comparison** — POPET (perceptron) vs. Intel's XPT physical-page
  tracker (`offchip_pred_type`).
- **Core vs. uncore placement** — the predictor can run inside the core
  (pre-translation, virtual address) or beside the LLC at the uncore
  (post-translation, physical address), selected by `offchip_pred_location`. XPT
  is only useful at the uncore with the physical address; with the physical
  address it refuses to run in the core.
- **DDRP action** — a speculative direct-DRAM-prefetch path driven by the
  prediction (`enable_ddrp`), studied at both placements.
- Reads ChampSim v1 and v2 traces; all knobs go through the X-macro framework in
  [inc/knobs.def](../inc/knobs.def).

Design entrypoints live in [docs/](../docs) — e.g.
[uncore-offchip-predictor-design.md](../docs/uncore-offchip-predictor-design.md),
[pcless-offchip-features.md](../docs/pcless-offchip-features.md),
[knob-framework-integration-design.md](../docs/knob-framework-integration-design.md).

## Project Rules

## Skills

Hermes keeps a small skill surface in phase 1:

- `git-commit` for review + verification + Lore commit preparation

These skills are intentionally slim. They should preserve the current
contract-first superproject layout and avoid inventing extra workflow layers.

Prefer current Git-tracked directory README files as navigation entrypoints.
Do not cite superseded paths or historical documents as the default source of
truth when a closer README or current design entrypoint exists.

## Guidelines

Please adhere to this guidelines whenever making changes to the code.

### 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

### 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.
