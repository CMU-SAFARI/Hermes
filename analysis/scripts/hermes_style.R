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
  "Hermes-NoPC"        = "#A1D99B",   # light green  (core predictor at uncore, PC-augmented features dropped)
  "Hermes-NoPC+Pythia" = "#238B45",   # strong green
  "Hermes-Core"           = "#BCBDDC",   # light purple (original PC-based, core placement)
  "Hermes-Core+Pythia"    = "#6A51A3",   # strong purple
  # Size study (own chart): blue = standalone (light->dark = Lite->Big),
  # orange = +Pythia (light->dark = Lite->Big).
  "Hermes-Lite"           = "#C6DBEF",
  "Hermes-Normal"         = "#6BAED6",
  "Hermes-Big"            = "#2171B5",
  "Hermes-Lite+Pythia"    = "#FDBE85",
  "Hermes-Normal+Pythia"  = "#FD8D3C",
  "Hermes-Big+Pythia"     = "#D94801",
  # Multi-prefetcher sensitivity study (Stride@L1D + Pythia@L2 baseline)
  "Stride+Pythia"              = "#4D4D4D",   # dark grey (2-prefetcher baseline)
  "Stride+Pythia+XPT"          = "#D94801",   # XPT strong orange
  "Stride+Pythia+Hermes-UnC"   = "#08519C",   # Hermes-UnC strong blue
  "Stride+Pythia+Hermes-Core"  = "#6A51A3"    # Hermes-Core strong purple
)

# Canonical display order (legend + dodge). Predictors-alone first, then
# Pythia, then the +Pythia combos. Subset per chart as needed. Colors are
# tied to config NAME (hermes_fill), so reordering here never changes a
# config's color — only its position.
hermes_levels <- c("nopref",
                   "XPT", "Hermes-UnC", "Hermes-NoPC", "Hermes-Core",
                   "Pythia",
                   "XPT+Pythia", "Hermes-UnC+Pythia",
                   "Hermes-NoPC+Pythia", "Hermes-Core+Pythia")

# Map raw ExpName tokens -> canonical display labels. Extend as batches add exps.
hermes_relabel <- function(x) {
  m <- c(
    "nopref"="nopref", "pythia"="Pythia", "xpt"="XPT",
    "xpt_pythia"="XPT+Pythia", "normal"="Hermes-UnC",
    "normal_pythia"="Hermes-UnC+Pythia",
    "pcless9"="Hermes-NoPC", "pcless9_pythia"="Hermes-NoPC+Pythia",
    "core_p"="Hermes-Core", "core_p_pythia"="Hermes-Core+Pythia",
    "core_o"="Hermes-Core-O", "core_o_pythia"="Hermes-Core-O+Pythia")
  x <- sub("^bw[0-9]+_", "", x)   # strip bwNNNN_ prefix
  x <- sub("^proj_", "", x); x <- sub("^c2_", "", x)
  out <- m[x]; ifelse(is.na(out), x, unname(out))
}

hermes_fill_scale <- function(...) ggplot2::scale_fill_manual(values = hermes_fill, name = NULL, ...)
