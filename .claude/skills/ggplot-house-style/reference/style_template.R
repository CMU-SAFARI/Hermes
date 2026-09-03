# ============================================================================
# HOUSE STYLE TEMPLATE  --  source() this from every chart script.
#
# HOW TO USE: copy this file into your project's analysis/scripts dir as (e.g.)
# `<project>_style.R`, then fill ONLY the three PROJECT STUBS at the top
# (palette, level order, relabel map) from your .plot-contract.yml. Everything
# below the stubs is the FIXED look-and-feel -- do not edit it per project, so
# every chart in the house reads as one system. See examples/hermes/hermes_style.R
# for a filled-in instance.
# ============================================================================

# ---------------------------------------------------------------------------
# PROJECT STUBS  --  the ONLY things you edit per project. Fill from the
# `palette:` / `labels:` blocks of .plot-contract.yml.
# ---------------------------------------------------------------------------

# 1. Palette: DISPLAY-name -> color. Colors are tied to the config NAME, so
#    reordering the legend never changes a series' color. Keep families
#    coherent (e.g. one hue per method; light = standalone, dark = combined).
house_fill <- c(
  "nopref" = "#CCCCCC"
  # "MethodA"          = "#9ECAE1",
  # "MethodA+Baseline" = "#08519C",
  # "Baseline"         = "#6E6E6E"
)

# 2. Canonical legend / dodge order (subset per chart as needed). Position
#    only; color stays tied to name via house_fill.
house_levels <- c("nopref"
  # , "MethodA", "Baseline", "MethodA+Baseline"
)

# 3. Relabel: raw data token (e.g. ExpName value) -> display name. Extend as
#    new experiment tokens appear. Unknown tokens pass through unchanged.
house_relabel <- function(x) {
  m <- c(
    "nopref" = "nopref"
    # , "methodA_raw" = "MethodA", "baseline_raw" = "Baseline"
  )
  # strip common run-prefixes before mapping (edit to match your naming), e.g.:
  # x <- sub("^bw[0-9]+_", "", x); x <- sub("^proj_", "", x)
  out <- m[x]
  ifelse(is.na(out), x, unname(out))
}

# ---------------------------------------------------------------------------
# FIXED LOOK-AND-FEEL  --  do NOT edit per project.
# ---------------------------------------------------------------------------

# Fill scale bound to the palette above.
house_fill_scale <- function(...) ggplot2::scale_fill_manual(values = house_fill, name = NULL, ...)
# Colour scale (for line charts) reusing the same palette.
house_colour_scale <- function(...) ggplot2::scale_colour_manual(values = house_fill, name = NULL, ...)

# The house theme: hrbrthemes Roboto-Condensed base, legend at bottom, axis
# titles horizontally centered, vertical gridlines dropped for bar charts.
theme_house <- function(base_size = 11, axis_title_size = 12, bars = TRUE) {
  th <- hrbrthemes::theme_ipsum_rc(base_size = base_size, axis_title_size = axis_title_size) +
    ggplot2::theme(
      legend.position = "bottom",
      axis.title.x = ggplot2::element_text(hjust = 0.5),
      axis.title.y = ggplot2::element_text(hjust = 0.5)
    )
  if (bars) th <- th + ggplot2::theme(panel.grid.major.x = ggplot2::element_blank())
  th
}

# Fixed geom defaults (call inside a ggplot()).
house_dodge   <- function(width = 0.9) ggplot2::position_dodge(width = width)
house_col     <- function(...) ggplot2::geom_col(width = 0.85, colour = "grey30", linewidth = 0.12, ...)
house_hline1  <- function(y = 1) ggplot2::geom_hline(yintercept = y, linetype = "dashed", colour = "grey40")

# Dual export: PNG via ragg @200dpi + one vector format at identical geometry.
# ALWAYS call this instead of a bare ggsave.
#   vector = "pdf" (default, cairo_pdf) or "svg" (cairo grDevices::svg).
# Both cairo devices convert glyphs to PATHS, so the house font renders
# identically on machines that do not have it installed. Do NOT swap in
# svglite: it writes live <text font-family="..."> without embedding the font,
# so any viewer lacking the font silently substitutes a different one.
house_save <- function(plot, path_noext, width = 13, height = 7, dpi = 200,
                       vector = c("pdf", "svg")) {
  vector <- match.arg(vector)
  ggplot2::ggsave(paste0(path_noext, ".png"), plot, width = width, height = height,
                  dpi = dpi, device = ragg::agg_png)
  if (vector == "pdf") {
    ggplot2::ggsave(paste0(path_noext, ".pdf"), plot, width = width, height = height,
                    device = grDevices::cairo_pdf)
  } else {
    ggplot2::ggsave(paste0(path_noext, ".svg"), plot, width = width, height = height,
                    device = grDevices::svg)
  }
  message("wrote ", path_noext, ".{png,", vector, "}")
}

# Benefit-of-doubt accuracy convention: a series that makes no prediction is
# credited 100% accuracy / 0% coverage (so it is not penalized as "wrong").
# Apply to a precision/accuracy vector after aggregation.
benefit_of_doubt_accuracy <- function(acc) ifelse(is.na(acc), 100, acc)
