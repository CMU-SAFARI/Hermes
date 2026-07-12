#!/usr/bin/env Rscript
# Off-chip predictor accuracy(=precision) & coverage(=recall), two facets,
# non-weighted arithmetic mean per SPEC category + AVG. Runs TWO variants:
#   standalone  -> XPT, Hermes-UnC
#   pythia      -> XPT+Pythia, Hermes-UnC+Pythia   (predictors ON TOP of Pythia)
# Benefit-of-doubt: a trace with NO prediction (precision undefined) counts
# as accuracy=100% (its coverage is 0% there anyway).
suppressPackageStartupMessages({
  library(yaml); library(ggplot2); library(hrbrthemes); library(dplyr); library(tidyr)
})
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))

all <- read.csv(file.path(here, "rollup_membound146_native.csv"), stringsAsFactors = FALSE)

# category tags
y <- yaml::read_yaml("/home/rahbera/thesis/runs/cluster/spec26.mpki2.yml")
tr <- list()
for (s in names(y)) for (it in y[[s]]) {
  nm <- names(it)[1]; tg <- it[[1]]$tags
  su <- intersect(c("specrate","specspeed"), tg); ty <- intersect(c("specint","specfp"), tg)
  tr[[length(tr)+1]] <- data.frame(TraceName=nm,
    category=paste0(su[1], "-", sub("spec","",ty[1])), stringsAsFactors=FALSE)
}
tagmap <- bind_rows(tr)

variants <- list(
  standalone = list(exps=c("bw3200_xpt","bw3200_normal"),
                    sub="Standalone predictors"),
  pythia     = list(exps=c("bw3200_xpt_pythia","bw3200_normal_pythia"),
                    sub="Predictors on top of Pythia")
)

for (vn in names(variants)) {
  v <- variants[[vn]]
  d <- all[all$ExpName %in% v$exps, ] %>%
    mutate(ExpName = hermes_relabel(ExpName),
           Accuracy = suppressWarnings(as.numeric(precision)),
           Coverage = suppressWarnings(as.numeric(recall)),
           Accuracy = ifelse(is.na(Accuracy), 100, Accuracy),   # benefit of doubt
           Coverage = ifelse(is.na(Coverage), 0, Coverage)) %>%
    inner_join(tagmap, by = "TraceName") %>%
    pivot_longer(c(Accuracy, Coverage), names_to = "metric", values_to = "val")

  grp <- bind_rows(d, mutate(d, category = "AVG"))
  agg <- grp %>%
    group_by(metric, category, ExpName) %>%
    summarise(mean = mean(val), n = n(), .groups = "drop") %>%
    mutate(category = factor(category, levels = c("specrate-fp","specrate-int",
                                                  "specspeed-fp","specspeed-int","AVG")),
           ExpName  = factor(ExpName, levels = intersect(hermes_levels, unique(ExpName))),
           metric   = factor(metric, levels = c("Accuracy","Coverage")))

  n_cat <- agg %>% distinct(category, n) %>% group_by(category) %>%
    summarise(n = max(n), .groups="drop")
  cat_lab <- setNames(sprintf("%s\n(n=%d)", n_cat$category, n_cat$n), as.character(n_cat$category))

  write.csv(agg, file.path(here,"charts",
            sprintf("accuracy_coverage_%s_membound146_bw3200_numbers.csv", vn)), row.names=FALSE)

  dodge <- position_dodge(width = 0.9)
  p <- ggplot(agg, aes(category, mean, fill = ExpName)) +
    geom_col(position = dodge, width = 0.8, colour = "grey25", linewidth = 0.15) +
    geom_text(data = filter(agg, category == "AVG"),
              aes(label = sprintf("%.1f", mean), group = ExpName),
              position = dodge, vjust = -0.4, size = 3.2, colour = "grey15") +
    facet_wrap(~ metric, ncol = 2) +
    hermes_fill_scale() +
    scale_x_discrete(labels = cat_lab) +
    scale_y_continuous(breaks = seq(0,100,25), limits = c(0,108), expand = expansion(mult=c(0,0))) +
    labs(title = "Off-chip predictor accuracy & coverage — 146 memory-intensive traces",
         subtitle = sprintf("%s, full window, 1 core, DDR-3200. Bars: non-weighted arithmetic mean per category (%%). No-prediction traces count as 100%% accuracy.", v$sub),
         x = NULL, y = "Percent (%)") +
    theme_ipsum_rc(base_size = 12, axis_title_size = 13) +
    theme(legend.position = "bottom", panel.grid.major.x = element_blank(),
          axis.title.y = element_text(hjust = 0.5),
          strip.text = element_text(face = "bold", size = 13)) +
    guides(fill = guide_legend(nrow = 1))

  ggsave(file.path(here,"charts", sprintf("accuracy_coverage_%s_membound146_bw3200.png", vn)), p,
         width = 15, height = 7, dpi = 200, device = ragg::agg_png)
  ggsave(file.path(here,"charts", sprintf("accuracy_coverage_%s_membound146_bw3200.pdf", vn)), p,
         width = 15, height = 7, device = cairo_pdf)
  message(sprintf("Wrote accuracy_coverage_%s_membound146_bw3200.{png,pdf}", vn))
}
