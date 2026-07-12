#!/usr/bin/env Rscript
# Adapted from runs/cluster/plot_speedup.R for the membound146_bw3200 rollup:
# 6 exps (nopref baseline + 5), owner category labels, GEOMEAN group.
suppressPackageStartupMessages({
  library(yaml); library(ggplot2); library(hrbrthemes)
  library(dplyr); library(tidyr)
})

here     <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))
csv_path <- file.path(here, "rollup_membound146_native.csv")
yml_path <- "/home/rahbera/thesis/runs/cluster/spec26.mpki2.yml"

stats <- read.csv(csv_path, stringsAsFactors = FALSE)
stats$Filter <- suppressWarnings(as.integer(stats$Filter))
bad <- sort(unique(stats$TraceName[is.na(stats$Filter) | stats$Filter != 1L]))
if (length(bad)) message(sprintf("Dropping %d filtered trace(s)", length(bad)))
stats <- stats[!stats$TraceName %in% bad, ]

y <- yaml::read_yaml(yml_path)
tag_rows <- list()
for (suite in names(y)) {
  for (item in y[[suite]]) {
    nm   <- names(item)[1]
    tags <- item[[1]]$tags
    suite_tag <- intersect(c("specrate", "specspeed"), tags)
    type_tag  <- intersect(c("specint",  "specfp"),    tags)
    tag_rows[[length(tag_rows) + 1]] <- data.frame(
      TraceName = nm,
      suite = ifelse(length(suite_tag), suite_tag[1], NA_character_),
      type  = ifelse(length(type_tag),  type_tag[1],  NA_character_),
      stringsAsFactors = FALSE)
  }
}
tagmap <- dplyr::bind_rows(tag_rows)
tagmap$category <- paste0(sub("spec", "spec", tagmap$suite), "-",
                          sub("spec", "", tagmap$type))     # specrate-int etc.

base <- stats %>% filter(ExpName == "bw3200_nopf") %>%
  select(TraceName, base_ipc = ipc)

exp_map <- c(bw3200_pythia        = "Pythia",
             bw3200_xpt           = "XPT",
             bw3200_xpt_pythia    = "XPT+Pythia",
             bw3200_normal        = "Hermes-UnC",
             bw3200_normal_pythia = "Hermes-UnC+Pythia")

sp <- stats %>%
  filter(ExpName != "bw3200_nopf") %>%
  inner_join(base,   by = "TraceName") %>%
  inner_join(tagmap, by = "TraceName") %>%
  mutate(speedup = ipc / base_ipc,
         ExpName = unname(exp_map[ExpName]))

exp_levels <- intersect(hermes_levels,
                        c("Pythia", "XPT", "XPT+Pythia", "Hermes-UnC", "Hermes-UnC+Pythia"))
cat_levels <- c("specrate-fp", "specrate-int",
                "specspeed-fp", "specspeed-int", "GEOMEAN")

sp_grp <- bind_rows(sp, mutate(sp, category = "GEOMEAN"))

agg <- sp_grp %>%
  group_by(category, ExpName) %>%
  summarise(n      = n(),
            mu_log = mean(log(speedup)),
            sd_log = sd(log(speedup)),
            .groups = "drop") %>%
  mutate(geomean = exp(mu_log),
         gsd     = exp(sd_log),
         lo      = exp(mu_log - sd_log),
         hi      = exp(mu_log + sd_log),
         category = factor(category, levels = cat_levels),
         ExpName  = factor(ExpName,  levels = exp_levels)) %>%
  arrange(category, ExpName)

n_per_cat <- agg %>% distinct(category, n_cat = n) %>%
  group_by(category) %>% summarise(n_cat = max(n_cat), .groups = "drop")
cat_lab <- setNames(sprintf("%s\n(n=%d)", n_per_cat$category, n_per_cat$n_cat),
                    as.character(n_per_cat$category))

write.csv(agg %>% select(category, ExpName, n, geomean, gsd, lo, hi),
          file.path(here, "charts", "speedup_membound146_bw3200_numbers.csv"),
          row.names = FALSE)

dodge <- position_dodge(width = 0.9)

p <- ggplot(agg, aes(category, geomean, fill = ExpName)) +
  geom_hline(yintercept = 1, linetype = "dashed", colour = "grey40") +
  geom_col(position = dodge, width = 0.85, colour = "grey25", linewidth = 0.15) +
  geom_text(data = filter(agg, category == "GEOMEAN"),
            aes(label = sprintf("%.3f", geomean), group = ExpName),
            position = dodge, vjust = -0.45, size = 3.2, colour = "grey15") +
  hermes_fill_scale() +
  scale_x_discrete(labels = cat_lab) +
  scale_y_continuous(breaks = scales::breaks_width(0.2),
                     expand = expansion(mult = c(0, 0.05))) +
  coord_cartesian(ylim = c(0.8, NA)) +
  labs(title    = "Geomean speedup over nopref — 146 memory-intensive traces",
       subtitle = "Full window, 1 core, DDR-3200. Bars: geomean of per-trace IPC speedup.",
       x = NULL, y = "Geomean speedup (×)") +
  theme_ipsum_rc(base_size = 12, axis_title_size = 13) +
  theme(legend.position = "bottom",
        panel.grid.major.x = element_blank(),
        axis.title.y = element_text(hjust = 0.5)) +
  guides(fill = guide_legend(nrow = 1))

ggsave(file.path(here, "charts", "speedup_membound146_bw3200.png"), p,
       width = 15, height = 8, dpi = 200, device = ragg::agg_png)
ggsave(file.path(here, "charts", "speedup_membound146_bw3200.pdf"), p,
       width = 15, height = 8, device = cairo_pdf)
message("Wrote charts/speedup_membound146_bw3200.{png,pdf} + numbers csv")
