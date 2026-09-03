# Design: row-open-only DRAM prefetch (Experiment 2)

Date: 2026-08-04
Owner: Rahul Bera
Status: design approved (conversationally) — implementing autonomously; launch gated on owner green light.

## 1. Motivation

Companion to the blind bandwidth-gated DDRP baseline (Experiment 1). For every
demand-load L2 miss (bandwidth-gated by the reused blind predictor), issue a DRAM
request that **opens (activates) the target row but performs no column read** —
no data transfer, no data-bus bandwidth, no MSHR fill, no return. A later demand
access to that now-open row is a row-buffer hit (`tCAS` only, saving `tRP+tRCD`),
cutting effective DRAM latency **without read-bandwidth overhead** — the key
contrast with Experiment 1. Faithful model: the activation still costs bank time
(contention) and competes for the real read queue, so speculative opens that
thrash banks can even hurt.

## 2. Clean factoring (no new gate code)

Same predictor, same gate, **only the DRAM action differs**:

| Config | predictor | gate | action |
|---|---|---|---|
| blind-full-read (exp 1) | blind | `bw_thresh` | full DDRP read (burns BW) |
| row-open (exp 2) | blind | `bw_thresh` | row activation only (~0 BW) |

Bandwidth gating is entirely the blind predictor's `bw < blind_ddrp_bw_thresh`
(no new gate code). The only new code is the **action**, selected by a knob.

## 3. Key code facts (verified 2026-08-04)

- Timing: `dram_controller.cc:191-195` — row-buffer hit = `tCAS`; miss =
  `tRP+tRCD+tCAS`. `bank_request[...].open_row` set at `schedule()` `:247` and
  persists across bank-free.
- Completion: `process()` `:307` charges the data bus (`:356-360`) then
  returns/buffers data (`:379-402`); frees bank + `remove_queue` (`:423-454`).
- The "DRAM read traffic" metric = `Channel_i_RQ_row_buffer_hit/miss`
  (`main.cc:782`) = `RQ[i].ROW_BUFFER_HIT/MISS`, incremented only in `process()`.
- The bandwidth measure = `rq_enqueue_count` (incremented in `add_rq` `:668`) →
  `epoch_enqueue_count / DRAM_DBUS_MAX_CAS` → `uncore.DRAM.bw` quartile
  (`main.cc:1734-1744`).
- Uncore DDRP issue builds the packet in `CACHE::issue_ddrp_request`
  (`offchip_pred.cc:317`, `fill_level=FILL_DDRP`, `type=PREFETCH`) → `add_rq`.
- `PACKET` is `inc/block.h:117`.

## 4. Implementation (6 edits, 4 files)

1. **`inc/block.h`** — add `uint8_t row_open;` to PACKET (near the other flags);
   init `row_open = 0;` in the constructor.
2. **`inc/knobs.def`** — `DEF_KNOB(ddrp_row_open, ddrp_row_open, bool, bool, false)`
   in the DDRP section (default false = inert; unset reproduces frozen behavior).
3. **`src/offchip_pred.cc`** — tag the DDRP packet with
   `row_open = knob::ddrp_row_open` in **both** `CACHE::issue_ddrp_request`
   (uncore, the one blind uses) and `O3_CPU::issue_ddrp_request` (core, for
   consistency), before `add_rq`.
4. **`src/dram_controller.cc` `schedule()`** (after the LATENCY computation, past
   the pseudo-direct block, before "this bank is now busy"):
   ```cpp
   if (queue->entry[index].row_open) {
     LATENCY = row_buffer_hit ? 0 : (tRP + tRCD);   // activation only, no tCAS
   }
   ```
   `open_row` is set unconditionally at `:247`, so the row opens either way.
5. **`src/dram_controller.cc` `process()`** (right after the bank-done check
   `:332-333`, before the data-bus check `:335`):
   ```cpp
   if (queue->entry[request_index].row_open) {
     // row now open; no data transfer -> no data bus, no fill, no RQ traffic
     // counters. Free the bank and drop; open_row persists.
     bank_request[op_channel][op_rank][op_bank].request_index  = -1;
     bank_request[op_channel][op_rank][op_bank].row_buffer_hit = 0;
     bank_request[op_channel][op_rank][op_bank].working        = false;
     bank_request[op_channel][op_rank][op_bank].is_write       = 0;
     bank_request[op_channel][op_rank][op_bank].is_read        = 0;
     scheduled_reads[op_channel]--;
     queue->remove_queue(&queue->entry[request_index], uncore.cycle);
     update_process_cycle(queue);
     return;
   }
   ```
6. **`src/dram_controller.cc` `add_rq()`** (`:668`) — exclude row-opens from the
   bandwidth measure: `if (!packet->row_open) rq_enqueue_count++;`.

## 5. Why the two exclusions matter (correctness)

- **`ROW_BUFFER_HIT/MISS` not incremented for row-opens** (edit 5 returns before
  `:423`): keeps the "DRAM read traffic" metric reflecting only real data reads,
  so row-open correctly shows ~0 extra read traffic, and keeps the real-read
  row-buffer hit-rate (the benefit signal) clean.
- **`rq_enqueue_count` excludes row-opens** (edit 6): otherwise row-opens would
  inflate the measured DRAM bandwidth that gates the blind predictor — a
  self-suppressing feedback loop (row-opens raise "bandwidth" → gate closes →
  fewer row-opens). Row-opens consume no data bandwidth, so they must not count.
- Row-opens DO still occupy an RQ slot + bank time (edits 4/5 keep the faithful
  contention): they compete for the read queue and delay other bank accesses.

## 6. Config & comparison

Experiment config = Experiment 1's chain plus `--ddrp_row_open=true`:
```
WBASE + hermes_base.ini --ddrp_req_latency=1 --offchip_pred_type=blind
  --offchip_pred_location=uncore --blind_ddrp_bw_thresh=3 --ddrp_row_open=true --llc_latency=40
```
Comparison (146 mem-intensive, full window, DDR-3200): nopref, Hermes-UnC,
blind-full-read (exp1, `ddrp_row_open=false`), row-open (exp2, `=true`) — at the
same `bw_thresh=3` (75%). Metrics: speedup over nopref; **% DRAM read-traffic
increase** (row-open ≈ 0 — the point); DRAM row-buffer hit rate (should rise);
IPC.

## 7. Verification plan

1. Build clean (glc).
2. Smoke (mcf): with `ddrp_row_open=true`, confirm (a) DDRP still fires
   (`LLC_DDRP_issued > 0`), (b) `Channel_0_RQ_row_buffer_hit+miss` does NOT jump
   vs nopref the way exp1's full-read does (near-zero read-traffic increase),
   (c) IPC changes vs nopref (row-open has an effect). Contrast with
   `ddrp_row_open=false` (exp1 full-read: read traffic jumps).
3. Regression: core-mode + non-row-open DDRP byte-identical to HEAD (the knob
   defaults false; the PACKET field + guards are inert when unset).
