#!/usr/bin/env Rscript
# Bandwidth-sweep line chart: geomean speedup over nopref (same-MTPS baseline)
# vs DRAM MTPS, one line per config. 146 memory-intensive traces, full window.
suppressPackageStartupMessages({
  library(ggplot2); library(hrbrthemes); library(dplyr); library(tidyr)
})
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))

stats <- read.csv(file.path(here, "rollup_bwsweep_membound146_native.csv"), stringsAsFactors = FALSE)
stats$mtps   <- as.integer(sub("^bw([0-9]+)_.*$", "\\1", stats$ExpName))
stats$config <- sub("^bw[0-9]+_", "", stats$ExpName)

base <- stats %>% filter(config == "nopf") %>% select(TraceName, mtps, base = ipc)
sp <- stats %>% filter(config != "nopf") %>%
  inner_join(base, by = c("TraceName","mtps")) %>%
  mutate(speedup = ipc / base, ExpName = hermes_relabel(ExpName))

agg <- sp %>% group_by(mtps, ExpName) %>%
  summarise(n = n(), geomean = exp(mean(log(speedup))), .groups = "drop")

lev <- intersect(hermes_levels, unique(agg$ExpName))
agg$ExpName <- factor(agg$ExpName, levels = lev)

write.csv(agg %>% arrange(ExpName, mtps),
          file.path(here,"charts","bwsweep_curve_membound146_numbers.csv"), row.names=FALSE)

# color scale reuses the canonical fills as line colors
pal <- hermes_fill[levels(agg$ExpName)]

p <- ggplot(agg, aes(mtps, geomean, colour = ExpName, group = ExpName)) +
  geom_hline(yintercept = 1, linetype = "dashed", colour = "grey50") +
  geom_line(linewidth = 1) +
  geom_point(size = 2.4) +
  scale_colour_manual(values = pal, name = NULL) +
  scale_x_continuous(trans = "log2", breaks = c(200,400,800,1600,3200,6400),
                     labels = c("200","400","800","1600","3200","6400")) +
  scale_y_continuous(breaks = scales::breaks_width(0.05)) +
  labs(title = "Speedup vs DRAM bandwidth — 146 memory-intensive traces",
       subtitle = "1 core, full window. Geomean of per-trace IPC speedup over nopref at the SAME MTPS. n=141/144/146/146/146/146 (low-BW cells partly dropped).",
       x = "DRAM data rate (MT/s, log scale)", y = "Geomean speedup over nopref (x)") +
  theme_ipsum_rc(base_size = 12, axis_title_size = 13) +
  theme(legend.position = "bottom", axis.title.x = element_text(hjust=0.5),
        axis.title.y = element_text(hjust=0.5)) +
  guides(colour = guide_legend(nrow = 1))

ggsave(file.path(here,"charts","bwsweep_curve_membound146.png"), p,
       width = 13, height = 8, dpi = 200, device = ragg::agg_png)
ggsave(file.path(here,"charts","bwsweep_curve_membound146.pdf"), p,
       width = 13, height = 8, device = cairo_pdf)
message("Wrote bwsweep_curve_membound146.{png,pdf}")
