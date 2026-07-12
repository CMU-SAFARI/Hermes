#!/usr/bin/env Rscript
# % increase in DRAM READ requests (row-buffer hit + miss; writes excluded)
# over nopref, per config. Non-weighted arithmetic mean per SPEC category + AVG.
suppressPackageStartupMessages({
  library(yaml); library(ggplot2); library(hrbrthemes); library(dplyr); library(tidyr)
})
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))

stats <- read.csv(file.path(here, "dram_reqs_membound146_bw3200_native.csv"), stringsAsFactors = FALSE)

y <- yaml::read_yaml("/home/rahbera/thesis/runs/cluster/spec26.mpki2.yml")
tr <- list()
for (s in names(y)) for (it in y[[s]]) {
  nm <- names(it)[1]; tg <- it[[1]]$tags
  su <- intersect(c("specrate","specspeed"), tg); ty <- intersect(c("specint","specfp"), tg)
  tr[[length(tr)+1]] <- data.frame(TraceName=nm,
    category=paste0(su[1],"-",sub("spec","",ty[1])), stringsAsFactors=FALSE)
}
tagmap <- bind_rows(tr)

base <- stats %>% filter(ExpName == "bw3200_nopf") %>% select(TraceName, base = dram_read_reqs)

d <- stats %>% filter(ExpName != "bw3200_nopf") %>%
  inner_join(base, by = "TraceName") %>%
  inner_join(tagmap, by = "TraceName") %>%
  mutate(pct_inc = (dram_read_reqs - base) / base * 100,
         ExpName = hermes_relabel(ExpName))

grp <- bind_rows(d, mutate(d, category = "AVG"))
agg <- grp %>%
  group_by(category, ExpName) %>%
  summarise(mean = mean(pct_inc), n = n(), .groups = "drop") %>%
  mutate(category = factor(category, levels = c("specrate-fp","specrate-int",
                                                "specspeed-fp","specspeed-int","AVG")),
         ExpName  = factor(ExpName, levels = intersect(hermes_levels, unique(ExpName))))

n_cat <- agg %>% distinct(category, n) %>% group_by(category) %>% summarise(n=max(n), .groups="drop")
cat_lab <- setNames(sprintf("%s\n(n=%d)", n_cat$category, n_cat$n), as.character(n_cat$category))

write.csv(agg, file.path(here,"charts","dram_increase_membound146_bw3200_numbers.csv"), row.names=FALSE)

dodge <- position_dodge(width = 0.9)
p <- ggplot(agg, aes(category, mean, fill = ExpName)) +
  geom_col(position = dodge, width = 0.85, colour = "grey25", linewidth = 0.15) +
  geom_text(data = filter(agg, category == "AVG"),
            aes(label = sprintf("%.1f", mean), group = ExpName),
            position = dodge, vjust = -0.4, size = 3.2, colour = "grey15") +
  hermes_fill_scale() +
  scale_x_discrete(labels = cat_lab) +
  scale_y_continuous(breaks = scales::breaks_width(10), expand = expansion(mult = c(0, 0.06))) +
  labs(title    = "DRAM read-request increase over nopref — 146 memory-intensive traces",
       subtitle = "Row-buffer hit+miss reads (writes excluded), full window, 1 core, DDR-3200. Bars: non-weighted arithmetic mean per category.",
       x = NULL, y = "DRAM read requests increase (%)") +
  theme_ipsum_rc(base_size = 12, axis_title_size = 13) +
  theme(legend.position = "bottom", panel.grid.major.x = element_blank(),
        axis.title.y = element_text(hjust = 0.5)) +
  guides(fill = guide_legend(nrow = 1))

ggsave(file.path(here,"charts","dram_increase_membound146_bw3200.png"), p,
       width = 15, height = 8, dpi = 200, device = ragg::agg_png)
ggsave(file.path(here,"charts","dram_increase_membound146_bw3200.pdf"), p,
       width = 15, height = 8, device = cairo_pdf)
message("Wrote dram_increase_membound146_bw3200.{png,pdf}")
