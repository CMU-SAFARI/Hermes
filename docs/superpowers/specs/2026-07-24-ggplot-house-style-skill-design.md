# Design: `ggplot-house-style` — a portable house-style R plotting skill

Date: 2026-07-24
Owner: Rahul Bera
Status: design approved (2026-07-24), pending implementation plan

## 1. Motivation

Across the Hermes tuning campaign we built ~18 R scripts
(`runs/cluster/plot_speedup.R`, `runs/tuning/analysis/scripts/*.R`) that
generate publication-quality charts sharing one consistent look-and-feel:
ggplot2 + `hrbrthemes::theme_ipsum_rc`, a shared style module
(`hermes_style.R`), dual PNG/PDF export, and a numbers-CSV behind every chart.

We want to transfer that capability to other Claude Code sessions — and
possibly other machines — as a reusable skill. The hard part is that the
*style* is stable but the *data* is not: a new project may not even produce
CSVs with the same columns (`TraceName, ExpName, ipc, …`). The skill must keep
the look-and-feel fixed while adapting cleanly to whatever data a project
emits.

## 2. Core idea: fixed grammar, per-project adapter

Every one of our scripts follows the same 10-step grammar:

1. `#!/usr/bin/env Rscript` + a header documenting Inputs/Outputs.
2. `suppressPackageStartupMessages({ ggplot2; hrbrthemes; dplyr; tidyr; yaml; ragg; scales })`.
3. `source(<style module>)` — single source of truth for color / order / labels.
4. Read a **rollup CSV** with a known schema (e.g. `TraceName, ExpName, ipc[, precision, recall], Filter`).
5. Join per-row metadata from a **metadata file** (e.g. a trace-list YAML with category tags / workload / weight).
6. Transform: baseline = the `nopf` row; `speedup = ipc / base`; aggregate (plain geomean / weighted geomean / arithmetic mean for precision-recall).
7. Relabel + order raw series names to display names via a `relabel()` map and a canonical level order.
8. **Write a `*_numbers.csv`** holding the numbers behind every bar/line (reproducibility).
9. Plot: `geom_col`/`geom_line` + a `*_fill_scale()` + `theme_ipsum_rc()` + dashed grey hline at y=1 + geomean/AVG data labels; **no error bars by default**.
10. **Dual export**: PNG via `ragg::agg_png` @200 dpi + PDF via `cairo_pdf`, fixed width/height.

Steps 1–3, 8, 9, 10 are **fixed** (the portable core). Steps 4–7 are
**data-dependent** (the project adapter). The skill encodes the fixed grammar
as reusable assets and establishes the adapter once per project via a
handshake, then persists it.

## 3. What ships (layout)

Canonical home is `~/skillhub` (a git repo, initialized as part of this work).
A copy travels inside the Hermes repo so the skill stays with the research.

```
~/skillhub/                                   # git init as part of this work; source of truth
  ggplot-house-style/
    SKILL.md                                  # orientation, runtime flow, rules
    reference/
      conventions.md                          # the 10-step grammar; fixed vs variable, verbatim rules
      style_template.R                        # theme + geom defaults + dual-export FIXED;
                                              #   palette / level-order / relabel = TEMPLATE stubs to fill
      setup_r_env.sh                          # install 7 pkgs + Roboto Condensed font + verify smoke-render
      contract_template.yml                   # schema for the persisted per-project data contract
    examples/hermes/                          # the real, working reference (worked example)
      hermes_style.R
      plot_speedup_membound146.R              # grouped-bar archetype
      plot_scurve_rateint_hermes.R            # line / S-curve archetype
      plot_gap_pcless_vs_unc_pythia.R         # sorted-tornado archetype
      plot_accuracy_coverage_membound146.R    # 2-facet archetype
    sync.sh                                   # one-liner: copy this skill dir into a target repo's .claude/skills/

Hermes/.claude/skills/ggplot-house-style/     # a plain copy of the above (source of truth = skillhub)
```

Rationale for the copy (not a symlink): a symlink does not survive
`git clone`, so a plain copy guarantees the skill is present when Hermes is
cloned on another machine. `sync.sh` re-copies from skillhub when the
canonical version changes.

## 4. The portable core

### 4.1 `style_template.R` (fixed look-and-feel)
Encodes the visual invariants so any project inherits the same look:

- `theme_ipsum_rc(base_size, axis_title_size)`, legend at bottom, axis titles
  horizontally centered (`hjust = 0.5`), `panel.grid.major.x` blank for bar charts.
- `geom_col(width = 0.85, colour = "grey30", linewidth = 0.12)` + `position_dodge(width = 0.9)`.
- Dashed grey reference line at `y = 1` (`geom_hline`).
- Data labels on the summary group (GEOMEAN/AVG), `angle = 90` for dense charts.
- Dual export: `ggsave(png, device = ragg::agg_png, dpi = 200)` + `ggsave(pdf, device = cairo_pdf)`, fixed width/height.
- **No error bars** unless explicitly requested.

Fixed in the template; **template stubs** the project fills:

- `pal` — a named `config → color` vector (the palette).
- `levels` — the canonical legend/dodge order.
- `relabel(x)` — raw-token → display-name map.
- `fill_scale()` — a thin `scale_fill_manual(values = pal)` helper.

### 4.2 `conventions.md`
Prose statement of the 10-step grammar and the non-negotiable rules
(numbers-CSV per chart, dual export, no error bars by default, benefit-of-doubt
accuracy convention where a no-prediction series counts as 100% accuracy /
0% coverage). This is what Claude reads to "orient."

### 4.3 `setup_r_env.sh`
1. Verify R is installed (`Rscript --version`).
2. Install the 7 packages if missing (ggplot2, hrbrthemes, dplyr, tidyr, yaml, ragg, scales).
3. Install Roboto Condensed (system font package or `hrbrthemes::import_roboto_condensed()`).
4. Smoke-render a one-bar chart through **both** devices (ragg PNG + cairo PDF) to confirm the toolchain works end-to-end.
5. Documented manual fallback for offline / locked-down machines (CRAN mirror may be blocked — we hit this in the sandbox; also `ggpattern` was uninstallable there, so patterns are optional, not required).

## 5. The project adapter (orient once, then reuse)

### 5.1 First use in a project — the handshake
Because a new project's data may not resemble Hermes rollups at all, Claude
must not guess. On first use it:

1. `head` the data file(s); read any metadata file.
2. Detect: the baseline row/definition, the series/label column(s), candidate
   metric column(s), and a plausible default aggregation.
3. **Echo the inferred data contract back to the user for confirmation /
   correction** before generating any script.
4. On confirmation, write `.plot-contract.yml` into the **analysis directory**
   (the directory that holds the rollup CSVs). A later session discovers it by
   walking up from the data path, so orientation is found without being told.

### 5.2 `.plot-contract.yml` (persisted contract)
Captures exactly the variable bits so later sessions skip the handshake:

```yaml
data:
  path: runs/tuning/analysis/rollup_*.csv   # file or glob
  schema: [TraceName, ExpName, ipc, precision, recall, Filter]
baseline:
  by: ExpName                                # column that identifies the baseline
  value: nopref                              # baseline row value; speedup = metric / baseline
metric:
  default: ipc
labels:                                      # raw series token -> display name
  hermes_uncore_phys_o: Hermes-UnC
  hermes_uncore_pcless9_o: Hermes-NoPC
metadata:                                    # optional per-row join
  file: runs/cluster/spec26.mpki2.yml
  join_key: TraceName
  provides: [category, workload, weight]
aggregation:
  default: geomean                           # geomean | weighted_geomean | mean
  weight_field: weight
palette:                                     # config -> color (fills style_template stubs)
  Hermes-UnC: "#9ECAE1"
  Hermes-NoPC: "#A1D99B"
output:
  dir: runs/tuning/analysis/charts
```

### 5.3 Subsequent use
Claude reads `.plot-contract.yml`, skips the handshake, and goes straight to
generating. If it detects schema drift (missing columns, new series tokens not
in the label map), it re-runs a targeted handshake and updates the contract.

## 6. Runtime flow (what `SKILL.md` directs)

1. **Orient** — read `conventions.md`; look for `.plot-contract.yml` in the project.
2. **Ensure env** — only if packages/font missing, run `setup_r_env.sh`; verify with the smoke-render.
3. **Adapter** — no contract → handshake → write contract; contract present → load it.
4. **Ask what to plot** — chart type + which series/configs + which metric; map to the nearest `examples/` archetype.
5. **Generate + run** — fill `style_template.R` (palette/levels/relabel from the contract) and a `plot_<name>.R` (steps 4–7 from the contract); produce PNG + PDF + `_numbers.csv`.
6. **Show + iterate.**

## 7. Chart archetypes (from the four shipped examples)

- **Grouped bars** — geomean/weighted-geomean speedup by category/workload with GEOMEAN group and data labels (`plot_speedup_membound146.R`).
- **Line / S-curve** — sorted per-item metric with a subtitle summary (min/max/top-N) (`plot_scurve_rateint_hermes.R`).
- **Sorted tornado** — signed per-item gap sorted, two-color by sign (`plot_gap_pcless_vs_unc_pythia.R`).
- **2-facet** — two metrics side by side (e.g. accuracy + coverage) sharing style (`plot_accuracy_coverage_membound146.R`).

## 8. Decided defaults

- **Name:** `ggplot-house-style`.
- **Examples:** the four archetypes above (not all ~18 scripts); `conventions.md` points to the full set in the Hermes repo.
- **Sync:** skillhub is source-of-truth; the Hermes copy is a plain `cp` via `sync.sh` (not a symlink, so it survives `git clone`).

## 9. Success criteria

1. On a machine with the environment set up, running the skill against the
   existing Hermes rollups reproduces a chart **visually equivalent** to the
   committed campaign charts (same theme, palette, export formats, numbers-CSV).
2. On a project whose data has **different columns**, the first-use handshake
   correctly elicits and persists a `.plot-contract.yml`, and a subsequent run
   generates an on-style chart **without** re-asking the schema.
3. `setup_r_env.sh` on a fresh machine (packages/font absent) ends with a
   passing smoke-render through both devices, or prints an actionable manual
   fallback when the network/CRAN is blocked.
4. The skill exists in both `~/skillhub` (git-tracked) and
   `Hermes/.claude/skills/`, and `sync.sh` reconciles them.

## 10. Out of scope

- No new chart types beyond the four archetypes (others are composed by Claude
  from the grammar at request time).
- No `ggpattern`/pattern-fill dependency (optional; was uninstallable in the
  sandbox — palettes use large-contrast sequential colors instead).
- No automated visual-regression/pixel-diff testing (criterion 1 is a
  human/visual check).
- The skill does not run simulations or roll up results; it consumes
  already-produced CSVs (that is the cluster-run / rollup skills' job).
