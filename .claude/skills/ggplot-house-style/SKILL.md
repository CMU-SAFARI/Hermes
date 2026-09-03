---
name: ggplot-house-style
description: Generate publication-quality charts in a consistent house style (ggplot2 + hrbrthemes, dual PNG/PDF export, a numbers-CSV behind every chart). Use when the user wants to plot/chart research results in R, reproduce the paper chart look, or set up the R plotting environment on a new machine. Orients to a project's data format once, persists it, then plots.
---

# House-style R plotting

Produce charts that read as one system — the same theme, palette, export
formats, and reproducibility artifacts as the Hermes paper figures — while
adapting to whatever data a project emits. The style is fixed; the data is
learned once per project and persisted.

Read `reference/conventions.md` before generating any chart. It defines the
10-step grammar and the non-negotiable rules. `examples/hermes/` holds four
real, working archetype scripts and their filled-in style module.

## Runtime flow

Create a todo per step and work them in order.

### 1. Orient
- Read `reference/conventions.md`.
- Look for a `.plot-contract.yml` by walking up from the user's data path (it
  lives in the analysis directory that holds the rollup CSVs). If found, the
  project is already oriented — load it and skip to step 4.

### 2. Ensure environment
- Run `bash reference/setup_r_env.sh --verify`. If it reports missing packages
  or a failed smoke-render, run `bash reference/setup_r_env.sh` (no flag) to
  install and verify. If install fails (offline/no CRAN), relay the script's
  MANUAL FALLBACK to the user — do not silently proceed with a broken toolchain.

### 3. Adapter — orient the project ONCE (only if no contract exists)
The new project's data may look nothing like Hermes rollups. Do not guess.
- `head` the data file(s); read any metadata file the user points to.
- Infer: the baseline row/definition, the series/label column, candidate metric
  column(s), a plausible default aggregation, and a metadata join if present.
- **Echo the inferred contract back to the user for confirmation/correction.**
  Show it as the `contract_template.yml` structure, filled from what you saw.
- On confirmation, copy `reference/contract_template.yml` into the analysis
  directory as `.plot-contract.yml` with the confirmed values, and copy
  `reference/style_template.R` into the project's scripts dir as
  `<project>_style.R`, filling its three PROJECT STUBS from the contract's
  `palette` / `order` / `labels`.

### 4. Ask what to plot
- Ask the user: which chart archetype (grouped bars / line-S-curve / sorted
  tornado / 2-facet — see conventions.md), which series/configs, which metric.
- Map the request to the nearest script in `examples/hermes/`.
- Note any vector-format preference (PDF default, SVG on request) — it changes
  only the `house_save(vector=)` argument.

### 5. Generate + run
- Copy the closest example as `plot_<name>.R` in the project's scripts dir.
- Rewire steps 4-7 (data read, metadata join, transform, relabel) from
  `.plot-contract.yml`; keep steps 1-3, 8-10 (grammar + export) unchanged.
- Run it with `Rscript`. Confirm it produced **PNG + the vector file +
  `*_numbers.csv`** in the contract's output dir. If any is missing, the chart
  is not done.

### 6. Look at it, then show + iterate
- **View the rendered chart yourself before surfacing it.** Read the PNG (or
  rasterize the vector file: `rsvg-convert -w 1000 chart.svg -o /tmp/check.png`)
  and check the two things code review cannot catch: dead whitespace beside the
  panel, and text that overflows or collides. Fix per "Canvas sizing" in
  conventions.md and re-run.
- For an SVG, also confirm the font is baked in: `grep -c 'font-family' chart.svg`
  must be 0 (see "Vector export").
- Surface the PNG to the user (SendUserFile). Iterate on labels/ranges/subtitle
  per feedback, re-running the script each time.

## Rules (from conventions.md — do not violate)
- No error bars unless explicitly requested.
- A `*_numbers.csv` behind every chart; PNG plus one vector format every time.
- Vector text must be paths (cairo devices), never an un-embedded font
  reference (svglite) — the font will not survive on another machine.
- No dead whitespace beside the panel; verify by viewing the rendered file.
- Color is tied to the series NAME (in `house_fill`), never its position.
- No dependency on `ggpattern` (may be unavailable); use contrast palettes.

## Where this skill lives
Canonical source is `~/skillhub/ggplot-house-style/`. A copy travels in each
consuming repo under `.claude/skills/ggplot-house-style/`. When the canonical
version changes, re-sync with `bash sync.sh <target-repo>`.
