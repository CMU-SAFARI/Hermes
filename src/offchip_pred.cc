#include <iostream>
#include <cassert>
#include "ooo_cpu.h"
#include "offchip_pred_base.h"
#include "offchip_pred_basic.h"
#include "offchip_pred_random.h"
#include "offchip_pred_perc.h"
#include "offchip_pred_hmp_local.h"
#include "offchip_pred_hmp_gshare.h"
#include "offchip_pred_hmp_gskew.h"
#include "offchip_pred_hmp_ensemble.h"
#include "offchip_pred_ttp.h"
#include "offchip_pred_xpt.h"
#include "offchip_pred_blind.h"
#include "knobs.h"

//=============================================================================
// Common: shared by the core path (O3_CPU) and the uncore path (LLC CACHE)
//=============================================================================

// Factory: build the predictor selected by offchip_pred_type and return it.
// Placement (core vs uncore) comes from knob::offchip_pred_location, kept
// consistent everywhere instead of passing an explicit owner string.
static OffchipPredBase *create_offchip_predictor(uint32_t cpu, string type,
                                                 uint64_t seed)
{
  // single, greppable log line: "Adding Offchip predictor: <type> at
  // <core/uncore>"
  cout << "Adding Offchip predictor: " << type << " at "
       << knob::offchip_pred_location << endl;

  // LLC-side (uncore) support is added incrementally; only the types that have
  // a working uncore PACKET* path may live at the LLC. Currently: "none" and
  // "xpt". Relax this further as perc/etc. gain their uncore implementations.
  if (!knob::offchip_pred_location.compare("uncore")) {
    assert((!type.compare("none") || !type.compare("xpt") ||
            !type.compare("perc") || !type.compare("blind")) &&
           "LLC-side offchip predictor currently supports only 'none', 'xpt', "
           "'perc', and 'blind'");
  }

  if (!type.compare("none")) {
    return new OffchipPredBase(cpu, type, seed);
  } else if (!type.compare("basic")) {
    return new OffchipPredBasic(cpu, type, seed);
  } else if (!type.compare("random")) {
    return new OffchipPredRandom(cpu, type, seed);
  } else if (!type.compare("perc")) {
    return new OffchipPredPerc(cpu, type, seed);
  } else if (!type.compare("hmp-local")) {
    return new OffchipPredHMPLocal(cpu, type, seed);
  } else if (!type.compare("hmp-gshare")) {
    return new OffchipPredHMPGshare(cpu, type, seed);
  } else if (!type.compare("hmp-gskew")) {
    return new OffchipPredHMPGskew(cpu, type, seed);
  } else if (!type.compare("hmp-ensemble")) {
    return new OffchipPredHMPEnsemble(cpu, type, seed);
  } else if (!type.compare("ttp")) {
    return new OffchipPredTTP(cpu, type, seed);
  } else if (!type.compare("xpt")) {
    return new OffchipPredXPT(cpu, type, seed);
  } else if (!type.compare("blind")) {
    return new OffchipPredBlind(cpu, type, seed);
  }
  return NULL;
}

//=============================================================================
// O3_CPU: core-side placement (offchip_pred_location == core)
//=============================================================================

void O3_CPU::initialize_offchip_predictor(uint64_t seed)
{
  offchip_pred = create_offchip_predictor(cpu, knob::offchip_pred_type, seed);
}

void O3_CPU::print_config_offchip_predictor()
{
  cout << "offchip_pred_type " << knob::offchip_pred_type << endl
       << "offchip_pred_mark_merged_load "
       << knob::offchip_pred_mark_merged_load << endl
       << "offchip_pred_location " << knob::offchip_pred_location << endl
       << endl;

  if (offchip_pred) {
    offchip_pred->print_config();
  }
}

void O3_CPU::dump_stats_offchip_predictor()
{
  float precision =
            (float)stats.offchip_pred.true_pos /
            (stats.offchip_pred.true_pos + stats.offchip_pred.false_pos),
        recall = (float)stats.offchip_pred.true_pos /
                 (stats.offchip_pred.true_pos + stats.offchip_pred.false_neg);

  cout << "Core_" << cpu << "_offchip_pred_pred_called "
       << stats.offchip_pred.pred_called << endl
       << "Core_" << cpu << "_offchip_pred_true_pos "
       << stats.offchip_pred.true_pos << endl
       << "Core_" << cpu << "_offchip_pred_false_pos "
       << stats.offchip_pred.false_pos << endl
       << "Core_" << cpu << "_offchip_pred_false_neg "
       << stats.offchip_pred.false_neg << endl
       << "Core_" << cpu << "_offchip_pred_precision " << precision * 100
       << endl
       << "Core_" << cpu << "_offchip_pred_recall " << recall * 100 << endl
       << endl;

  if (offchip_pred) {
    offchip_pred->dump_stats();
  }
}

void O3_CPU::offchip_predictor_update_dram_bw(uint8_t dram_bw)
{
  if (offchip_pred) {
    offchip_pred->update_dram_bw(dram_bw);
  }
}

/* This function is called at every LLC eviction.
 * Cache-level prediction mechanisms that operate as tag-tracking mechanisms
 * need to track LLC evictions to make accurate off-chip predictions.
 * This function is written to track such evictions */
void O3_CPU::offchip_predictor_track_llc_eviction(uint32_t set, uint32_t way,
                                                  uint64_t address)
{
  if (offchip_pred && !knob::offchip_pred_type.compare("ttp")) {
    OffchipPredTTP *ocp_lp = (OffchipPredTTP *)offchip_pred;
    ocp_lp->track_llc_eviction(address);
  }
}

// Core-side stats + training, run on LQ release (release_load_queue).
void O3_CPU::offchip_pred_stats_and_train(uint32_t lq_index)
{
  // stats
  stats.offchip_pred.pred_called++;
  if (LQ.entry[lq_index].went_offchip == 1 &&
      LQ.entry[lq_index].went_offchip_pred == 1)  // true positive
  {
    stats.offchip_pred.true_pos++;
  } else if (LQ.entry[lq_index].went_offchip == 0 &&
             LQ.entry[lq_index].went_offchip_pred == 1)  // false negative
  {
    stats.offchip_pred.false_pos++;
  } else if (LQ.entry[lq_index].went_offchip == 1 &&
             LQ.entry[lq_index].went_offchip_pred == 0)  // false negative
  {
    stats.offchip_pred.false_neg++;
  }

  // training
  uint32_t rob_index  = LQ.entry[lq_index].rob_index;
  int32_t  data_index = -1;
  for (int32_t index = 0; index < NUM_INSTR_SOURCES; ++index) {
    if (ROB.entry[rob_index].lq_index[index] == lq_index) {
      data_index = index;
      break;
    }
  }
  assert(data_index != -1);
  if (offchip_pred) {
    offchip_pred->train(&ROB.entry[rob_index], (uint32_t)data_index,
                        &LQ.entry[lq_index]);
  }
}

// Core-side speculative direct-DRAM (DDRP) fetch: issued when the prediction
// says off-chip.
void O3_CPU::issue_ddrp_request(uint32_t lq_index, uint32_t call_type)
{
  stats.ddrp.total++;
  assert(LQ.entry[lq_index].translated == COMPLETED);
  assert(LQ.entry[lq_index].physical_address != 0);
  assert(knob::enable_ddrp);

  // check if DDRP is forcefully disabled by DDRP monitor
  if (ddrp_monitor && ddrp_monitor->disable_ddrp == true) {
    return;
  }

  if (dram_controller->get_occupancy(1, LQ.entry[lq_index].physical_address >>
                                            LOG2_BLOCK_SIZE) ==
      dram_controller->get_size(1,
                                LQ.entry[lq_index].physical_address >>
                                    LOG2_BLOCK_SIZE))  // check RQ's occupancy
  {
    stats.ddrp.dram_rq_full++;
    return;
  }

  // add it to DRAM_CONTROLLER's MSHR
  PACKET data_packet;
  data_packet.fill_level = FILL_DDRP;
  data_packet.fill_l1d   = 0;
  data_packet.cpu        = cpu;
  data_packet.data_index = LQ.entry[lq_index].data_index;
  data_packet.lq_index   = lq_index;
  data_packet.address = LQ.entry[lq_index].physical_address >> LOG2_BLOCK_SIZE;
  data_packet.full_addr     = LQ.entry[lq_index].physical_address;
  data_packet.instr_id      = LQ.entry[lq_index].instr_id;
  data_packet.rob_index     = LQ.entry[lq_index].rob_index;
  data_packet.rob_position  = LQ.entry[lq_index].rob_position;
  data_packet.rob_part_type = LQ.entry[lq_index].rob_part_type;
  data_packet.ip            = LQ.entry[lq_index].ip;
  data_packet.type          = PREFETCH;
  data_packet.row_open      = knob::ddrp_row_open;
  data_packet.asid[0]       = LQ.entry[lq_index].asid[0];
  data_packet.asid[1]       = LQ.entry[lq_index].asid[1];
  data_packet.event_cycle =
      LQ.entry[lq_index].event_cycle + knob::ddrp_req_latency;

  DDRP_DP(if (warmup_complete[data_packet.cpu]) {
    cout << "[CORE_DDRP_REQ] " << __func__
         << " instr_id: " << data_packet.instr_id << " address: " << hex
         << data_packet.address;
    cout << " full_addr: " << data_packet.full_addr << dec;
    cout << " current: " << current_core_cycle[data_packet.cpu]
         << " event: " << data_packet.event_cycle << endl;
  });

  dram_controller->add_rq(&data_packet);
  stats.ddrp.issued[call_type]++;
}

//=============================================================================
// CACHE: uncore-side placement (offchip_pred_location == uncore; LLC-owned)
//=============================================================================

// The LLC owns a single predictor instance (per-core ooo_cpu[i].offchip_pred
// stay NULL).
void CACHE::initialize_offchip_predictor(uint64_t seed)
{
  offchip_pred = create_offchip_predictor(cpu, knob::offchip_pred_type, seed);
}

// cache.h only forward-declares OffchipPredBase, so it cannot make this call.
void CACHE::reset_offchip_predictor_stats()
{
  if (offchip_pred) {
    offchip_pred->reset_stats();
  }
}

void CACHE::offchip_predictor_update_dram_bw(uint8_t dram_bw)
{
  if (offchip_pred) {
    offchip_pred->update_dram_bw(dram_bw);
  }
}

void CACHE::print_config_offchip_predictor()
{
  cout << "offchip_pred_type " << knob::offchip_pred_type << endl
       << "offchip_pred_mark_merged_load "
       << knob::offchip_pred_mark_merged_load << endl
       << "offchip_pred_location " << knob::offchip_pred_location << endl
       << "offchip_pred_llc_mshr_merged_load_as_offchip "
       << knob::offchip_pred_llc_mshr_merged_load_as_offchip << endl
       << endl;

  if (offchip_pred) {
    offchip_pred->print_config();
  }
}

// Uncore mode: the LLC owns the predictor, so it also reports the accuracy
// stats (LLC_offchip_pred_*) plus the predictor's own internal counters.
void CACHE::dump_stats_offchip_predictor()
{
  auto &predict = stats.offchip_pred.predict;
  auto &train   = stats.offchip_pred.train;

  float precision = (float)train.true_pos / (train.true_pos + train.false_pos),
        recall    = (float)train.true_pos / (train.true_pos + train.false_neg);

  cout << "LLC_offchip_pred_predict_called " << predict.called << endl << endl;

  // Counted at the train sites: accuracy, then which site supplied the ground
  // truth and the label it carried. A predict/train gap wider than the requests
  // in flight at an ROI edge means a site released a load without resolving it.
  cout << "LLC_offchip_pred_train_called " << train.called << endl
       << "LLC_offchip_pred_true_pos " << train.true_pos << endl
       << "LLC_offchip_pred_false_pos " << train.false_pos << endl
       << "LLC_offchip_pred_false_neg " << train.false_neg << endl
       << "LLC_offchip_pred_precision " << precision * 100 << endl
       << "LLC_offchip_pred_recall " << recall * 100 << endl
       << "LLC_offchip_pred_train_llc_hit_onchip " << train.llc_hit[0] << endl
       << "LLC_offchip_pred_train_llc_hit_offchip " << train.llc_hit[1] << endl
       << "LLC_offchip_pred_train_llc_miss_onchip " << train.llc_miss[0] << endl
       << "LLC_offchip_pred_train_llc_miss_offchip " << train.llc_miss[1]
       << endl
       << "LLC_offchip_pred_train_llc_rq_merge_onchip " << train.llc_rq_merge[0]
       << endl
       << "LLC_offchip_pred_train_llc_rq_merge_offchip "
       << train.llc_rq_merge[1] << endl
       << "LLC_offchip_pred_train_llc_wq_fwd_onchip " << train.llc_wq_fwd[0]
       << endl
       << "LLC_offchip_pred_train_llc_wq_fwd_offchip " << train.llc_wq_fwd[1]
       << endl
       << "LLC_offchip_pred_train_llc_mshr_merge_onchip "
       << train.llc_mshr_merge[0] << endl
       << "LLC_offchip_pred_train_llc_mshr_merge_offchip "
       << train.llc_mshr_merge[1] << endl
       << endl;

  // LLC-owned DDRP (speculative direct-DRAM) stats (mirrors the core's
  // Core_*_DDRP_*)
  cout << "LLC_DDRP_total " << stats.ddrp.total << endl
       << "LLC_DDRP_issued " << stats.ddrp.issued << endl
       << "LLC_DDRP_dram_RQ_full " << stats.ddrp.dram_rq_full << endl
       << "LLC_DDRP_dram_MSHR_full " << stats.ddrp.dram_mshr_full << endl
       << endl;

  if (offchip_pred) {
    offchip_pred->dump_stats();
  }
}

// Sole entry point for the uncore predict site. Called on the L2C as it
// forwards a demand load to `llc`'s RQ: predicting here rather than at LLC
// dequeue hides the LLC RQ queuing latency. The prediction and the feature
// state ride on the packet into the LLC RQ; `llc` owns the predictor and trains
// it once the tag lookup resolves. On a positive prediction, fire the
// speculative direct-DRAM fetch -- same gate as the core side
// (ooo_cpu.cc:2123).
void CACHE::offchip_pred_predict(PACKET *packet, CACHE *llc)
{
  if (cache_type != IS_L2C || !llc->offchip_pred ||
      knob::offchip_pred_location != "uncore" || !packet->is_data ||
      packet->type != LOAD) {
    return;
  }

  llc->stats.offchip_pred.predict.called++;
  packet->went_offchip_pred = llc->offchip_pred->predict(packet);

  if (packet->went_offchip_pred && knob::enable_ddrp) {
    llc->issue_ddrp_request(packet);
  }
}

// Uncore train path: mirror of O3_CPU::offchip_pred_stats_and_train. The
// hit/miss outcome (packet->went_offchip) is set by the caller in
// CACHE::handle_read; the prediction (packet->went_offchip_pred) + feature
// state were set earlier by predict().
void CACHE::offchip_pred_stats_and_train(PACKET *packet)
{
  // accuracy bookkeeping owned by the LLC (same TP/FP/FN scheme as the core
  // path)
  if (packet->went_offchip && packet->went_offchip_pred) {
    stats.offchip_pred.train.true_pos++;
  } else if (!packet->went_offchip && packet->went_offchip_pred) {
    stats.offchip_pred.train.false_pos++;
  } else if (packet->went_offchip && !packet->went_offchip_pred) {
    stats.offchip_pred.train.false_neg++;
  }

  // train the LLC-owned predictor, then release the per-request feature state
  if (offchip_pred) {
    offchip_pred->train(packet);
  }
  if (packet->ocp_feature) {
    delete packet->ocp_feature;
    packet->ocp_feature = NULL;
  }
}

// Sole entry point for the uncore train sites. `went_offchip` is the ground
// truth the site resolved; `site` counts it. A demand load reaches DRAM only on
// an LLC miss -- an LLC hit, an RQ merge and a WQ forward are all on-chip.
// Every site that releases a demand load must call this, or the prediction
// escapes the accuracy counters and its feature state is never freed.
void CACHE::offchip_pred_resolve(PACKET *packet, bool went_offchip,
                                 uint64_t (&site)[2])
{
  if (cache_type != IS_LLC || !offchip_pred ||
      knob::offchip_pred_location != "uncore" || !packet->is_data ||
      packet->type != LOAD) {
    return;
  }

  packet->went_offchip = went_offchip;
  stats.offchip_pred.train.called++;
  site[went_offchip]++;
  offchip_pred_stats_and_train(packet);
}

// Uncore analog of O3_CPU::issue_ddrp_request: on a positive uncore prediction,
// issue the speculative direct-DRAM fetch for `packet`. Same logic as the core
// version (stats, DDRP monitor, DRAM RQ occupancy check) but, since we already
// have the request's PACKET, we generate a fresh DRAM packet populating only a
// minimal field set (like prefetch_line). Stats + DDRP monitor are per-core
// (this serves packet->cpu); DRAM is the LLC's lower_level.
void CACHE::issue_ddrp_request(PACKET *packet)
{
  uint32_t ddrp_cpu = packet->cpu;

  stats.ddrp.total++;
  assert(packet->full_addr != 0);
  assert(knob::enable_ddrp);

  // check if DDRP is forcefully disabled by the (per-core) DDRP monitor
  if (ooo_cpu[ddrp_cpu].ddrp_monitor &&
      ooo_cpu[ddrp_cpu].ddrp_monitor->disable_ddrp == true) {
    return;
  }

  // DDRP goes straight to DRAM (bypassing the cache hierarchy), so query this
  // cache's direct line to the DRAM controller (linked in main.cc). Check its
  // RQ occupancy first.
  if (dram_controller->get_occupancy(1, packet->address) ==
      dram_controller->get_size(1, packet->address)) {
    stats.ddrp.dram_rq_full++;
    return;
  }

  // build a minimal DDRP packet (only the fields the DRAM path needs), like
  // prefetch_line
  PACKET ddrp_packet;
  ddrp_packet.fill_level = FILL_DDRP;
  ddrp_packet.fill_l1d   = 0;
  ddrp_packet.cpu        = ddrp_cpu;
  ddrp_packet.address    = packet->address;
  ddrp_packet.full_addr  = packet->full_addr;
  ddrp_packet.ip         = packet->ip;
  ddrp_packet.type       = PREFETCH;
  ddrp_packet.row_open   = knob::ddrp_row_open;
  ddrp_packet.event_cycle =
      current_core_cycle[ddrp_cpu] + knob::ddrp_req_latency;

  DDRP_DP(if (warmup_complete[ddrp_cpu]) {
    cout << "[UNCORE_DDRP_REQ] " << __func__
         << " instr_id: " << ddrp_packet.instr_id << " address: " << hex
         << ddrp_packet.address;
    cout << " full_addr: " << ddrp_packet.full_addr << dec;
    cout << " current: " << current_core_cycle[ddrp_cpu]
         << " event: " << ddrp_packet.event_cycle << endl;
  });

  dram_controller->add_rq(&ddrp_packet);
  stats.ddrp.issued++;
}
