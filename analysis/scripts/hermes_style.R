# Canonical Hermes paper style — SOURCE THIS from every chart script so bar
# colors/order stay identical across all figures. Edit here ONCE to restyle
# every chart. (No error bars by default; add per-chart only if asked.)

# Family-coherent sequential palette: grey = Pythia/baseline, orange = XPT
# family, blue = Hermes family. Within a family, "alone" = light endpoint,
# "+Pythia" = strong dark endpoint (large-contrast, not subtle shades).
hermes_fill <- c(
  "nopref"            = "#CCCCCC",
  "Pythia"            = "#6E6E6E",
  "XPT"               = "#FDBE85",   # light orange
  "XPT+Pythia"        = "#D94801",   # strong orange-red
  "Hermes-UnC"        = "#9ECAE1",   # light blue
  "Hermes-UnC+Pythia" = "#08519C",   # strong dark blue
  # Hermes size variants (for future size charts; greens, coherent within-set)
  "Hermes-UnC-Lite"   = "#A1D99B",
  "Hermes-UnC-Big"    = "#238B45"
)

# Canonical display order (legend + dodge). Subset per chart as needed.
hermes_levels <- c("nopref", "Pythia", "XPT", "XPT+Pythia",
                   "Hermes-UnC", "Hermes-UnC+Pythia",
                   "Hermes-UnC-Lite", "Hermes-UnC-Big")

# Map raw ExpName tokens -> canonical display labels. Extend as batches add exps.
hermes_relabel <- function(x) {
  m <- c(
    "nopref"="nopref", "pythia"="Pythia", "xpt"="XPT",
    "xpt_pythia"="XPT+Pythia", "normal"="Hermes-UnC",
    "normal_pythia"="Hermes-UnC+Pythia",
    "lite"="Hermes-UnC-Lite", "big"="Hermes-UnC-Big",
    "lite_pythia"="Hermes-UnC-Lite+Pythia", "big_pythia"="Hermes-UnC-Big+Pythia")
  x <- sub("^bw[0-9]+_", "", x)   # strip bwNNNN_ prefix
  x <- sub("^proj_", "", x); x <- sub("^c2_", "", x)
  out <- m[x]; ifelse(is.na(out), x, unname(out))
}

hermes_fill_scale <- function(...) ggplot2::scale_fill_manual(values = hermes_fill, name = NULL, ...)
