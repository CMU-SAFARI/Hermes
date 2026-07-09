# CAMPAIGN 1 — CLOSEOUT — Hermes-Big / Normal / Lite (uncore off-chip predictor)

Closed 2026-07-09. Owner: Rahul Bera. Operator: Claude (autonomous).
Code frozen the entire campaign at Hermes b18be0c (single-core binary).

## The three shipping configurations

All three: features 20,21,22 (PageSpatialFootprint + PageMissRatio +
LastNDeltas), hash 2, region 2MB, act +2, train +8/−21, uncore placement,
physical address, page-buffer assoc 16. Base: config/ocp_hermes.ini.

**Hermes-Lite — 4.25 KB** (PB 8x16=128e + weights 2048/1024/1024)
```
--ocp_perc_activated_features=20,21,22 --ocp_perc_weight_array_sizes=2048,1024,1024 --ocp_perc_feature_hash_types=2,2,2 --ocp_perc_feature_region_size_log2s=21,21,21 --ocp_perc_activation_threshold=2 --ocp_perc_pos_train_thresh=8 --ocp_perc_neg_train_thresh=-21 --ocp_perc_page_buf_sets=8 --ocp_perc_page_buf_assoc=16 --offchip_pred_location=uncore --ocp_perc_use_physical_address=true
```
**Hermes-Normal — 15.1 KB** (PB 32x16=512e + weights 8192/1024/4096)
```
(same with --ocp_perc_weight_array_sizes=8192,1024,4096 --ocp_perc_page_buf_sets=32)
```
**Hermes-Big — 29.6 KB** (PB 64x16=1024e + weights 8192/1024/16384)
```
(same with --ocp_perc_weight_array_sizes=8192,1024,16384 --ocp_perc_page_buf_sets=64)
```

Storage amortization argument: original per-core Hermes = 4KB/core = 16KB
per 4-core chip. Normal (16KB-class) = storage parity as ONE shared uncore
structure; Lite = 4x cheaper; Big = 2x richer.

## Headline (E3', 146 traces; prec% / recall% / geo-IPC vs nopf / vs Pythia)

| config | window | prec | recall | vs nopf | vs Pythia |
|--------|--------|------|--------|---------|-----------|
| Lite | tuning (50M+200M) | 84.4 | 67.4 | 1.1607 | 1.0146 |
| Lite | **full (100M+500M)** | **84.4** | **67.8** | **1.1691** | **1.0162** |
| Normal | tuning | 85.2 | 69.8 | 1.1609 | 1.0148 |
| Normal | **full** | **85.4** | **69.8** | **1.1708** | **1.0176** |
| Big | tuning | 85.7 | 69.7 | 1.1618 | 1.0156 |
| Big | **full** | **85.8** | **69.7** | **1.1713** | **1.0180** |
| 6K-sizes ref (E2 pick, PB 64x16) | full | 85.1 | 69.3 | 1.1712 | 1.0180 |
| 64k-uniform ref (~148KB) | full (145*) | 86.4 | 71.5 | 1.1736 | 1.0187 |

*64k ref: ns3-187B hit the 12h wall (the campaign's slowest trace); scored
on 145. All other rows 146/146, zero failures.

Reading: a clean monotone ladder at full scale — Big gives +1.4pp precision
and +1.9pp recall over Lite for 25KB more; Big sits 0.6pp precision /
1.8pp recall / 0.07% geo below the 148KB unconstrained reference at 1/5th
the storage. Tuning-vs-full rows agree within 0.2pp accuracy — the
shipping configs are window-stable, directly measured.

## Evidence chain

r01 (56 singleton features) -> r02 (265 pairs; beam converged +0.171% <
0.2%) -> owner precision-first amendment (bandwidth-masking evidence:
fotonik3d/graph500 correlations +0.975/+0.954) -> sweep01/02 (triple
f20+f21+f22 dominates; 9-pt act curve) -> sweep03 (pos_train=8 precision
lever) -> sweep04 (f21 size-insensitive) -> val01/02 (147-trace
confirmation) -> val03 (full-window PASS; fidelity tripwire retired) -> E1
(op-point act+2 tr8/-21 confirmed at FW, 146t) -> E2 (weight caps) -> E4
(joint PB x weight Pareto, 51 cfg: PB-dominance hypothesis inverted —
weights ~2x more byte-efficient for precision; geometry-insensitive;
Big/Normal/Lite picked on 32/16/4KB budget lines) -> E3' (this headline).

## Campaign totals and incidents

- Jobs: 38,379 of the 120,000 authorization (32%). ~17 batches + retries.
  Zero unrecovered data loss.
- Incidents, all recovered and documented in round decisions: backfill
  starvation (24h default walltime -> right-sizing discipline); 3h trim
  killed 71 jobs at ~88% (-> 4h floor rule); FIFO tie-break starvation (->
  Nice=200 on diagnostic twins); node-loss ID-block failures x3 (retries);
  sibling-filter contamination in infra rollups (-> own-health rebuild
  rule); OPERATOR ERROR: e3 _fw twins re-ran the tuning window (sed missed
  the TBASE macro definition; 292 duplicate jobs ~0.24% budget; fw refs
  re-run correctly in E3' via the WBASE method with pre-submit assertions).
- Selection integrity: every pick precision-first per the owner amendment;
  all rounds published to origin/tuning with per-round decision.md.

## Owner checklist

1. Accept Hermes-Big/Normal/Lite as campaign-1's shipping configurations
   (CLI lines above; full per-trace data in rounds/e3p/ + batch CSVs).
2. Campaign 2 (multi-core, quad-core binary) is STAGED separately:
   fixed-window simulator verified 4/4 criteria and re-pinned (ef3b36a3);
   mix100 tlist + exp_quad drafted; PENDING YOUR DECISION: uncore per-core
   precision/recall attribution (currently structurally unmeasured — see
   campaign2/PLAN.md and the final-review residual-threats note) + DRAM
   channel count + solo-reference definition.
3. Multi-core acceptance criterion on record: in-mix per-workload P/R ==
   solo within noise under embed_cpu_id=true; off/on A-B quantifies the
   embedding.
