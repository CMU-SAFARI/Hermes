#!/usr/bin/env Rscript
suppressPackageStartupMessages({ library(yaml); library(ggplot2); library(hrbrthemes); library(dplyr) })
here <- "/home/rahbera/thesis/runs/tuning/analysis"
source(file.path(here, "scripts", "hermes_style.R"))
stats <- read.csv(file.path(here,"rollup_sens_multipf_native.csv"), stringsAsFactors=FALSE)
y <- yaml::read_yaml("/home/rahbera/thesis/runs/cluster/spec26.mpki2.yml"); tr<-list()
for (s in names(y)) for (it in y[[s]]) { nm<-names(it)[1]; tg<-it[[1]]$tags
  su<-intersect(c("specrate","specspeed"),tg); ty<-intersect(c("specint","specfp"),tg)
  tr[[length(tr)+1]]<-data.frame(TraceName=nm, category=paste0(su[1],"-",sub("spec","",ty[1])), stringsAsFactors=FALSE) }
tagmap<-bind_rows(tr)
lab <- c(sens_stride_pythia_xpt="Stride+Pythia+XPT",
         sens_stride_pythia_unc="Stride+Pythia+Hermes-UnC",
         sens_stride_pythia_core="Stride+Pythia+Hermes-Core")
lev <- c("Stride+Pythia+XPT","Stride+Pythia+Hermes-UnC","Stride+Pythia+Hermes-Core")
base <- stats %>% filter(ExpName=="sens_stride_pythia") %>% select(TraceName, b=ipc)
sp <- stats %>% filter(ExpName %in% names(lab)) %>% inner_join(base,by="TraceName") %>%
  inner_join(tagmap,by="TraceName") %>% mutate(speedup=ipc/b, ExpName=unname(lab[ExpName]))
grp <- bind_rows(sp, mutate(sp, category="GEOMEAN"))
agg <- grp %>% group_by(category,ExpName) %>% summarise(n=n(), geomean=exp(mean(log(speedup))), .groups="drop") %>%
  mutate(category=factor(category,levels=c("specrate-fp","specrate-int","specspeed-fp","specspeed-int","GEOMEAN")),
         ExpName=factor(ExpName,levels=lev))
n_cat<-agg%>%distinct(category,n)%>%group_by(category)%>%summarise(n=max(n),.groups="drop")
cat_lab<-setNames(sprintf("%s\n(n=%d)",n_cat$category,n_cat$n),as.character(n_cat$category))
write.csv(agg, file.path(here,"charts","sens_incremental_numbers.csv"), row.names=FALSE)
dodge<-position_dodge(width=0.9)
p<-ggplot(agg,aes(category,geomean,fill=ExpName))+
  geom_hline(yintercept=1,linetype="dashed",colour="grey40")+
  geom_col(position=dodge,width=0.8,colour="grey25",linewidth=0.15)+
  geom_text(data=filter(agg,category=="GEOMEAN"),aes(label=sprintf("%.3f",geomean),group=ExpName),
            position=dodge,vjust=0.5,hjust=-0.15,size=3.6,colour="grey15",angle=90)+
  hermes_fill_scale()+scale_x_discrete(labels=cat_lab)+
  scale_y_continuous(breaks=scales::breaks_width(0.005),expand=expansion(mult=c(0,0.06)))+
  coord_cartesian(ylim=c(0.99,NA))+
  labs(title="Multi-prefetcher sensitivity: what each predictor adds ON TOP OF Stride+Pythia",
       subtitle="146 mem-intensive traces, full window, 1 core, DDR-3200. Bars: geomean per-trace speedup over the Stride@L1D + Pythia@L2 baseline.",
       x=NULL,y="Geomean speedup over Stride+Pythia")+
  theme_ipsum_rc(base_size=12,axis_title_size=13)+
  theme(legend.position="bottom",panel.grid.major.x=element_blank(),axis.title.y=element_text(hjust=0.5))+
  guides(fill=guide_legend(nrow=1))
ggsave(file.path(here,"charts","sens_incremental_membound146.png"),p,width=15,height=8,dpi=200,device=ragg::agg_png)
ggsave(file.path(here,"charts","sens_incremental_membound146.pdf"),p,width=15,height=8,device=cairo_pdf)
message("wrote sens_incremental")
