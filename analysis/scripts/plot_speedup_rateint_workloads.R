#!/usr/bin/env Rscript
# Full SPEC Rate-Int per-workload speedup over nopref. Within a workload:
# SimPoint-WEIGHTED geomean of per-trace speedups. GEOMEAN group: non-weighted
# geomean of the per-workload speedups. 5 configs, canonical palette.
suppressPackageStartupMessages({
  library(yaml); library(ggplot2); library(hrbrthemes); library(dplyr); library(tidyr)
})
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))

stats <- read.csv(file.path(here, "rollup_rateint_projection_native.csv"), stringsAsFactors = FALSE)

# workload + weight from the rate-int tlist
d <- yaml::read_yaml("/home/rahbera/thesis/runs/cluster/spec26.rate-int.yml")
key <- names(d)[1]; rows <- list()
for (it in d[[key]]) {
  nm <- names(it)[1]
  rows[[length(rows)+1]] <- data.frame(TraceName = nm,
    workload = it[[1]]$workload, weight = as.numeric(it[[1]]$weight),
    stringsAsFactors = FALSE)
}
tw <- bind_rows(rows)

base <- stats %>% filter(ExpName == "proj_nopf") %>% select(TraceName, base = ipc)
sp <- stats %>% filter(ExpName != "proj_nopf") %>%
  inner_join(base, by = "TraceName") %>% inner_join(tw, by = "TraceName") %>%
  mutate(speedup = ipc / base, ExpName = hermes_relabel(ExpName))

# per-workload weighted geomean
wl <- sp %>% group_by(workload, ExpName) %>%
  summarise(geomean = exp(sum(weight * log(speedup)) / sum(weight)),
            n = n(), .groups = "drop")
# GEOMEAN: unweighted geomean of the per-workload speedups
geo <- wl %>% group_by(ExpName) %>%
  summarise(geomean = exp(mean(log(geomean))), n = sum(n), .groups = "drop") %>%
  mutate(workload = "GEOMEAN")

agg <- bind_rows(wl, geo) %>%
  mutate(short = ifelse(workload == "GEOMEAN", "GEOMEAN", sub("^[0-9]+\\.", "", workload)))
wl_order <- c(sort(unique(sub("^[0-9]+\\.", "", wl$workload))), "GEOMEAN")
agg$short   <- factor(agg$short, levels = wl_order)
agg$ExpName <- factor(agg$ExpName, levels = intersect(hermes_levels, unique(agg$ExpName)))

write.csv(agg %>% select(workload, short, ExpName, n, geomean),
          file.path(here,"charts","speedup_rateint_workloads_numbers.csv"), row.names=FALSE)

dodge <- position_dodge(width = 0.9)
p <- ggplot(agg, aes(short, geomean, fill = ExpName)) +
  geom_hline(yintercept = 1, linetype = "dashed", colour = "grey40") +
  geom_col(position = dodge, width = 0.85, colour = "grey30", linewidth = 0.12) +
  geom_text(data = filter(agg, short == "GEOMEAN"),
            aes(label = sprintf("%.3f", geomean), group = ExpName),
            position = dodge, vjust = -0.45, size = 2.6, colour = "grey15",
            angle = 90, hjust = -0.1) +
  hermes_fill_scale() +
  scale_y_continuous(breaks = scales::breaks_width(0.05), expand = expansion(mult=c(0,0.05))) +
  coord_cartesian(ylim = c(0.95, NA)) +
  labs(title = "SPEC Rate-Int per-workload speedup over nopref (full suite)",
       subtitle = "Full window, 1 core, DDR-3200. Per workload: SimPoint-weighted geomean of trace speedups. GEOMEAN: non-weighted geomean across workloads.",
       x = NULL, y = "Speedup over nopref (x)") +
  theme_ipsum_rc(base_size = 11, axis_title_size = 12) +
  theme(legend.position = "bottom", panel.grid.major.x = element_blank(),
        axis.text.x = element_text(angle = 40, hjust = 1),
        axis.title.y = element_text(hjust = 0.5)) +
  guides(fill = guide_legend(nrow = 1))

ggsave(file.path(here,"charts","speedup_rateint_workloads.png"), p,
       width = 18, height = 8, dpi = 200, device = ragg::agg_png)
ggsave(file.path(here,"charts","speedup_rateint_workloads.pdf"), p,
       width = 18, height = 8, device = cairo_pdf)
message("Wrote speedup_rateint_workloads.{png,pdf}")
