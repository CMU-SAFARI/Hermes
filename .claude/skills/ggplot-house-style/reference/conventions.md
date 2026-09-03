# House-style plotting conventions

This is the grammar every chart script follows. Steps 1-3, 8, 9, 10 are
**fixed** (the portable look-and-feel, provided by `style_template.R`). Steps
4-7 are **data-dependent** (filled from the project's `.plot-contract.yml`).

## The 10-step grammar

1. `#!/usr/bin/env Rscript` + a header comment documenting **Inputs** and **Outputs**.
2. `suppressPackageStartupMessages({ library(ggplot2); library(hrbrthemes); library(dplyr); library(tidyr); library(yaml); library(ragg); library(scales) })`.
3. `source(<project>_style.R)` — the single source of truth for color / order / labels (a filled copy of `style_template.R`).
4. **Read the rollup CSV** named in the contract, with its declared schema.
5. **Join per-row metadata** from the contract's metadata file (category / workload / weight), if any.
6. **Transform**: baseline = the contract's baseline row; `speedup = metric / baseline`; aggregate per the contract's default (`geomean` | `weighted_geomean` | `mean`).
7. **Relabel + order** raw series tokens to display names via `house_relabel()` and factor with `house_levels`.
8. **Write a `*_numbers.csv`** holding the exact numbers behind every bar/line — same basename as the chart, in the output dir. Non-negotiable (reproducibility).
9. **Plot**: `house_col()` / `geom_line()` + `house_fill_scale()` (or `house_colour_scale()`) + `theme_house()` + `house_hline1()` at y=1 + data labels on the summary group.
10. **Dual export** via `house_save(p, path_noext)` — PNG (ragg @200dpi) + one vector format at identical geometry (`vector = "pdf"` default, `"svg"` on request).

## Non-negotiable rules (the house look-and-feel)

- **No error bars** unless the user explicitly asks for them.
- **A numbers-CSV behind every chart** (step 8). If you cannot write it, the chart is not done.
- **PNG plus one vector format**, every time, via `house_save()` — PDF by default, SVG when the user asks. Never PNG alone.
- **Vector text must be paths, never a font reference** — see "Vector export" below.
- **No dead whitespace beside the panel** — see "Canvas sizing" below. Look at the rendered file before calling a chart done.
- **Color is tied to the series NAME**, not its position — defined once in `house_fill`. Reordering a chart never recolors a series. This is what makes a whole paper's figures cohere.
- **Data labels** go on the summary group only (the `GEOMEAN` / `AVG` bar), `angle = 90` when the chart is dense.
- **Benefit-of-doubt accuracy**: a series that makes no prediction counts as **100% accuracy / 0% coverage**, never as "wrong" (`benefit_of_doubt_accuracy()`).
- **Aggregation discipline**: within a group use the contract's aggregation (weighted geomean for per-workload speedup, arithmetic mean for precision/recall); across groups the summary is a non-weighted geomean/mean unless told otherwise. State which in the subtitle.
- **Palettes, not patterns**: prefer large-contrast sequential colors within a family. `ggpattern` is optional and may be unavailable (it was uninstallable in our sandbox) — never make a chart depend on it.

## Vector export: SVG and the font trap

The house theme uses a specific font (Roboto Condensed). How the vector device
writes text decides whether that font survives on someone else's machine.

| Device | Text in the file | Font fidelity | Editable text |
|---|---|---|---|
| `grDevices::svg` / `cairo_pdf` (cairo) | glyphs as **vector paths** | correct everywhere | no |
| `svglite::svglite` | live `<text font-family="Roboto Condensed">`, **font not embedded** | substituted wherever the font is missing | yes |

**Default to cairo.** A figure that ships to a paper, a slide deck, or a
colleague must not depend on their font set. Only reach for `svglite` when the
user explicitly needs to edit the text in Illustrator/Inkscape — and then embed
the font as a base64 `@font-face`, or the same substitution bites.

Verify rather than assume — a cairo SVG has no font references at all:

```bash
grep -c 'font-family' chart.svg   # cairo: 0    svglite: one per text element
grep -c '<text'       chart.svg   # cairo: 0 (paths)
```

Trade-off to state when it matters: cairo files are ~10x larger (paths cost
more bytes than characters).

**Continuous colourbars rasterize.** Both devices emit a small `<image>` for a
smooth `scale_fill_gradient` legend strip — tiles and text stay vector. If a
venue demands 100% vector, switch the legend to `guide_colorsteps()` (discrete
solid rectangles).

## Canvas sizing: kill the whitespace

Wide margins of empty canvas beside the panel are a defect, not a neutral
default. Two distinct causes:

1. **A pinned panel aspect.** `coord_equal()` / `coord_fixed()` (heatmaps,
   square-tile grids) fixes the panel's width:height. The panel *cannot*
   stretch to fill a wider canvas, so every extra inch becomes side whitespace.
   Fix by matching the canvas to the panel, not the reverse: for an
   `ncol x nrow` tile grid,
   `width ≈ height_available * (ncol/nrow) + y-axis + legend`.
   A 6x5 heatmap at `height = 6.6` wants `width ≈ 8`, not 10.
2. **Generous theme defaults.** `theme_ipsum_rc` ships roomy margins, and a
   right-hand legend sits far from the panel. Tighten with:

```r
theme(plot.margin        = margin(t = 6, r = 6, b = 4, l = 4),
      legend.box.spacing = unit(3, "pt"),   # default is ~11pt
      legend.margin      = margin(0, 0, 0, 0))
```

Non-`coord_equal` charts (bars, lines) stretch to fill, so for them only cause
2 applies — plus simply oversized `width`/`height` arguments.

**Always look at the output.** Rasterize the vector file and view it; do not
judge whitespace from the code:

```bash
rsvg-convert -w 1000 chart.svg -o /tmp/check.png   # or: chromium --headless --screenshot
```

## Chart archetypes (see `examples/hermes/`)

- **Grouped bars** — `plot_speedup_membound146.R`: geomean/weighted-geomean by category/workload with a `GEOMEAN` group and value labels.
- **Line / S-curve** — `plot_scurve_rateint_hermes.R`: per-item metric sorted low→high, subtitle summary (`min: … ; max: … ; top-N: …`).
- **Sorted tornado** — `plot_gap_pcless_vs_unc_pythia.R`: signed per-item gap sorted, two-color by sign, `geom_hline` at 0.
- **2-facet** — `plot_accuracy_coverage_membound146.R`: two metrics (accuracy + coverage) side by side sharing the style.

Any other chart is composed from this grammar at request time — do not invent a new styling system.

## The full reference set

These four are curated archetypes. The complete campaign chart library
(~18 scripts) lives in `runs/tuning/analysis/scripts/` in the Hermes repo,
with the canonical `hermes_style.R` alongside — consult it for a less common
chart shape before building one from scratch.
