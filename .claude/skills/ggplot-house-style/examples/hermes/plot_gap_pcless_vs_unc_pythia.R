#!/usr/bin/env Rscript
# Per-trace performance GAP: (Hermes-UnC+Pythia over Pythia) minus
# (Hermes-NoPC+Pythia over Pythia), in pp, all mem-intensive traces sorted.
# Positive = the full 3-feature uncore predictor wins; negative = 1-feature
# NoPC wins. Shows the spread hidden by the near-equal averages.
suppressPackageStartupMessages({ library(ggplot2); library(hrbrthemes); library(dplyr) })
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))
d <- read.csv(file.path(here, "gap_pcless_vs_unc_pythia_numbers.csv"), stringsAsFactors = FALSE)
d <- d %>% arrange(gap_pp) %>% mutate(rank = row_number(),
        winner = ifelse(gap_pp >= 0, "Hermes-UnC+Pythia wins", "Hermes-NoPC+Pythia wins"))
pal <- c("Hermes-UnC+Pythia wins" = hermes_fill[["Hermes-UnC+Pythia"]],
         "Hermes-NoPC+Pythia wins" = hermes_fill[["Hermes-NoPC+Pythia"]])
sub <- sprintf(paste0("%d traces, gap = 3-feat uncore minus 1-feat NoPC (speedup over Pythia).  ",
   "median %.2f, mean %.2f pp; range [%.2f, %.2f]; wins: UnC %d, tie %d, NoPC %d."),
   nrow(d), median(d$gap_pp), mean(d$gap_pp), min(d$gap_pp), max(d$gap_pp),
   sum(d$gap_pp>0.1), sum(abs(d$gap_pp)<=0.1), sum(d$gap_pp< -0.1))
p <- ggplot(d, aes(rank, gap_pp, fill = winner)) +
  geom_hline(yintercept = 0, colour = "grey40") +
  geom_col(width = 1) +
  scale_fill_manual(values = pal, name = NULL) +
  scale_y_continuous(breaks = scales::breaks_width(1)) +
  scale_x_continuous(expand = expansion(mult = c(0.01,0.01))) +
  labs(title = "Hermes-UnC vs NoPC (both + Pythia) — per-trace performance gap",
       subtitle = sub,
       x = "Memory-intensive traces (sorted by gap)",
       y = "Gap in speedup over Pythia (percentage points)") +
  theme_ipsum_rc(base_size = 11, axis_title_size = 12) +
  theme(legend.position = "bottom", axis.title.x = element_text(hjust=0.5), axis.title.y = element_text(hjust=0.5))
ggsave(file.path(here,"charts","gap_pcless_vs_unc_pythia.png"), p, width=13, height=7, dpi=200, device=ragg::agg_png)
ggsave(file.path(here,"charts","gap_pcless_vs_unc_pythia.pdf"), p, width=13, height=7, device=cairo_pdf)
message("wrote gap_pcless_vs_unc_pythia")
