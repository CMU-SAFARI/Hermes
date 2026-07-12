#!/usr/bin/env Rscript
# Hermes-UnC size study: Lite / Normal / Big, standalone and +Pythia, plus
# Pythia. Geomean speedup over nopref, per SPEC category + GEOMEAN. Same axes
# as the main speedup chart. Sources: combined CSV assembled by the caller
# (rollup_sizes_membound146_bw3200.csv, native format; all fc49264 binary).
suppressPackageStartupMessages({
  library(yaml); library(ggplot2); library(hrbrthemes); library(dplyr); library(tidyr)
})
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))

stats <- read.csv(file.path(here, "rollup_sizes_membound146_bw3200.csv"), stringsAsFactors = FALSE)

y <- yaml::read_yaml("/home/rahbera/thesis/runs/cluster/spec26.mpki2.yml")
tr <- list()
for (s in names(y)) for (it in y[[s]]) {
  nm <- names(it)[1]; tg <- it[[1]]$tags
  su <- intersect(c("specrate","specspeed"), tg); ty <- intersect(c("specint","specfp"), tg)
  tr[[length(tr)+1]] <- data.frame(TraceName=nm,
    category=paste0(su[1],"-",sub("spec","",ty[1])), stringsAsFactors=FALSE)
}
tagmap <- bind_rows(tr)

lab <- c(pythia="Pythia",
         lite="Hermes-Lite", normal="Hermes-Normal", big="Hermes-Big",
         lite_pythia="Hermes-Lite+Pythia", normal_pythia="Hermes-Normal+Pythia",
         big_pythia="Hermes-Big+Pythia")
lev <- c("Hermes-Lite","Hermes-Normal","Hermes-Big","Pythia",
         "Hermes-Lite+Pythia","Hermes-Normal+Pythia","Hermes-Big+Pythia")

base <- stats %>% filter(ExpName == "nopref") %>% select(TraceName, base = ipc)
sp <- stats %>% filter(ExpName != "nopref") %>%
  inner_join(base, by = "TraceName") %>% inner_join(tagmap, by = "TraceName") %>%
  mutate(speedup = ipc / base, ExpName = unname(lab[ExpName]))

grp <- bind_rows(sp, mutate(sp, category = "GEOMEAN"))
agg <- grp %>% group_by(category, ExpName) %>%
  summarise(n = n(), geomean = exp(mean(log(speedup))), .groups="drop") %>%
  mutate(category = factor(category, levels=c("specrate-fp","specrate-int",
                                              "specspeed-fp","specspeed-int","GEOMEAN")),
         ExpName  = factor(ExpName, levels = lev))

n_cat <- agg %>% distinct(category, n) %>% group_by(category) %>% summarise(n=max(n), .groups="drop")
cat_lab <- setNames(sprintf("%s\n(n=%d)", n_cat$category, n_cat$n), as.character(n_cat$category))
write.csv(agg, file.path(here,"charts","speedup_sizes_membound146_bw3200_numbers.csv"), row.names=FALSE)

dodge <- position_dodge(width = 0.9)
p <- ggplot(agg, aes(category, geomean, fill = ExpName)) +
  geom_hline(yintercept = 1, linetype = "dashed", colour = "grey40") +
  geom_col(position = dodge, width = 0.85, colour = "grey25", linewidth = 0.15) +
  geom_text(data = filter(agg, category == "GEOMEAN"),
            aes(label = sprintf("%.3f", geomean), group = ExpName),
            position = dodge, vjust = -0.45, size = 2.9, colour = "grey15") +
  hermes_fill_scale() +
  scale_x_discrete(labels = cat_lab) +
  scale_y_continuous(breaks = scales::breaks_width(0.2), expand = expansion(mult=c(0,0.05))) +
  coord_cartesian(ylim = c(0.8, NA)) +
  labs(title = "Hermes-UnC size comparison (Lite / Normal / Big) — 146 memory-intensive traces",
       subtitle = "Full window, 1 core, DDR-3200. Bars: geomean of per-trace IPC speedup over nopref.",
       x = NULL, y = "Geomean speedup (x)") +
  theme_ipsum_rc(base_size = 12, axis_title_size = 13) +
  theme(legend.position = "bottom", panel.grid.major.x = element_blank(),
        axis.title.y = element_text(hjust = 0.5)) +
  guides(fill = guide_legend(nrow = 1))

ggsave(file.path(here,"charts","speedup_sizes_membound146_bw3200.png"), p,
       width = 15, height = 8, dpi = 200, device = ragg::agg_png)
ggsave(file.path(here,"charts","speedup_sizes_membound146_bw3200.pdf"), p,
       width = 15, height = 8, device = cairo_pdf)
message("Wrote speedup_sizes_membound146_bw3200.{png,pdf}")
