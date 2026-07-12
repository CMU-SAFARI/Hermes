#!/usr/bin/env Rscript
# S-curve: per-trace speedup of Hermes-UnC STANDALONE over nopref, all 144
# SPEC Rate-Int traces sorted low->high. Shows the spread / any tail.
suppressPackageStartupMessages({
  library(ggplot2); library(hrbrthemes); library(dplyr)
})
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))

stats <- read.csv(file.path(here, "rollup_rateint_projection_native.csv"), stringsAsFactors = FALSE)
base <- stats %>% filter(ExpName == "proj_nopf") %>% select(TraceName, base = ipc)
sp <- stats %>% filter(ExpName == "proj_normal") %>%
  inner_join(base, by = "TraceName") %>%
  mutate(speedup = ipc / base) %>%
  arrange(speedup) %>% mutate(rank = row_number())

write.csv(sp %>% select(rank, TraceName, speedup),
          file.path(here,"charts","scurve_rateint_hermes_numbers.csv"), row.names = FALSE)

# top-N (highest-speedup) summary for the subtitle
topstat <- function(N) { g <- tail(sp$speedup, N)
  sprintf("Top-%d: %.3f-%.3f (geo %.3f)", N, min(g), max(g), exp(mean(log(g)))) }
sub <- paste0("Full window, 1 core, DDR-3200. All 144 traces sorted by speedup.  ",
              topstat(10), " . ", topstat(50), " . ", topstat(100), ".")

col <- "#2171B5"   # Hermes-UnC (blue family)
p <- ggplot(sp, aes(rank, speedup)) +
  geom_hline(yintercept = 1, linetype = "dashed", colour = "grey45") +
  geom_line(colour = col, linewidth = 1) +
  geom_point(colour = col, size = 1.1) +
  scale_y_continuous(breaks = scales::breaks_width(0.05)) +
  scale_x_continuous(expand = expansion(mult = c(0.01, 0.01))) +
  coord_cartesian(ylim = c(0.98, NA)) +
  labs(title = "Hermes-UnC standalone — per-trace speedup S-curve (SPEC Rate-Int)",
       subtitle = sub,
       x = "Rate-Int traces (sorted by speedup)", y = "Speedup over no-prefetching") +
  theme_ipsum_rc(base_size = 12, axis_title_size = 13) +
  theme(axis.title.x = element_text(hjust = 0.5), axis.title.y = element_text(hjust = 0.5))

ggsave(file.path(here,"charts","scurve_rateint_hermes.png"), p,
       width = 13, height = 7, dpi = 200, device = ragg::agg_png)
ggsave(file.path(here,"charts","scurve_rateint_hermes.pdf"), p,
       width = 13, height = 7, device = cairo_pdf)
message("Wrote scurve_rateint_hermes.{png,pdf}")
