#include <iostream>
#include <algorithm>
#include <cassert>
#include "offchip_pred_perc.h"
#include "util.h"
#include "ooo_cpu.h"
#include "knobs.h"

#if 0
#define MYLOG(cond, ...)                                    \
  if (cond) {                                               \
    fprintf(stdout, "[%25s@%3u] ", __FUNCTION__, __LINE__); \
    fprintf(stdout, __VA_ARGS__);                           \
    fprintf(stdout, "\n");                                  \
    fflush(stdout);                                         \
  }
#else
#define MYLOG(cond, ...) \
  {                      \
  }
#endif

//=============================================================================
// Common: config, stats, construction, and shared helpers (both placements)
//=============================================================================

void OffchipPredPerc::print_config()
{
  cout << "ocp_perc_use_physical_address "
       << knob::ocp_perc_use_physical_address << endl
       << "ocp_perc_activated_features "
       << print_activated_features(knob::ocp_perc_activated_features) << endl
       << "ocp_perc_weight_array_sizes "
       << array_to_string(knob::ocp_perc_weight_array_sizes) << endl
       << "ocp_perc_feature_hash_types "
       << array_to_string(knob::ocp_perc_feature_hash_types) << endl
       << "ocp_perc_activation_threshold "
       << knob::ocp_perc_activation_threshold << endl
       << "ocp_perc_max_weight " << knob::ocp_perc_max_weight << endl
       << "ocp_perc_min_weight " << knob::ocp_perc_min_weight << endl
       << "ocp_perc_pos_weight_delta " << knob::ocp_perc_pos_weight_delta
       << endl
       << "ocp_perc_neg_weight_delta " << knob::ocp_perc_neg_weight_delta
       << endl
       << "ocp_perc_pos_train_thresh " << knob::ocp_perc_pos_train_thresh
       << endl
       << "ocp_perc_neg_train_thresh " << knob::ocp_perc_neg_train_thresh
       << endl
       << "ocp_perc_page_buf_sets " << knob::ocp_perc_page_buf_sets << endl
       << "ocp_perc_page_buf_assoc " << knob::ocp_perc_page_buf_assoc << endl
       << "ocp_perc_last_n_load_pcs " << knob::ocp_perc_last_n_load_pcs << endl
       << "ocp_perc_last_n_pcs " << knob::ocp_perc_last_n_pcs << endl
       << "ocp_perc_enable_dynamic_act_thresh "
       << knob::ocp_perc_enable_dynamic_act_thresh << endl
       << "ocp_perc_update_act_thresh_epoch "
       << knob::ocp_perc_update_act_thresh_epoch << endl
       << "ocp_perc_high_critical_dram_bw_level "
       << knob::ocp_perc_high_critical_dram_bw_level << endl
       << "ocp_perc_low_critical_dram_bw_level "
       << knob::ocp_perc_low_critical_dram_bw_level << endl
       << "ocp_perc_poor_precision_thresh "
       << knob::ocp_perc_poor_precision_thresh << endl
       << "ocp_perc_act_thresh_update_gradient "
       << knob::ocp_perc_act_thresh_update_gradient << endl
       << "ocp_perc_min_activation_threshold "
       << knob::ocp_perc_min_activation_threshold << endl
       << "ocp_perc_max_activation_threshold "
       << knob::ocp_perc_max_activation_threshold << endl
       << endl;
}

void OffchipPredPerc::dump_stats()
{
  cout << "ocp_perc_train_called " << stats.train.called << endl
       << "ocp_perc_predict_called " << stats.predict.called << endl
       << "ocp_perc_predict_offchip " << stats.predict.outcome[1] << endl
       << "ocp_perc_predict_not_offchip " << stats.predict.outcome[0] << endl
       << endl
       << "ocp_perc_unique_pages " << unique_pages.size() << endl
       << "ocp_perc_page_buf_lookup_called " << stats.page_buf.called << endl
       << "ocp_perc_page_buf_lookup_hit " << stats.page_buf.hit << endl
       << "ocp_perc_page_buf_lookup_eviction " << stats.page_buf.eviction
       << endl
       << "ocp_perc_page_buf_lookup_insertion " << stats.page_buf.insertion
       << endl
       << endl
       << "ocp_perc_act_thresh_update_called " << stats.act_thresh_update.called
       << endl
       << "ocp_perc_act_thresh_update_increment "
       << stats.act_thresh_update.increment << endl
       << "ocp_perc_act_thresh_update_decrement "
       << stats.act_thresh_update.decrement << endl
       << "ocp_perc_act_thresh_update_max_observed_thresh "
       << stats.act_thresh_update.max_observed_thresh << endl
       << "ocp_perc_act_thresh_update_min_observed_thresh "
       << stats.act_thresh_update.min_observed_thresh << endl
       << endl;

  perc_pred->dump_stats();
}

void OffchipPredPerc::reset_stats()
{
  bzero(&stats, sizeof(stats));
  stats.act_thresh_update.max_observed_thresh = -999999999;
  stats.act_thresh_update.min_observed_thresh = 999999999;
  perc_pred->reset_stats();
}

OffchipPredPerc::OffchipPredPerc(uint32_t _cpu, string _type, uint64_t _seed)
    : OffchipPredBase(_cpu, _type, _seed)
{
  // The perceptron OCP can index by the physical address only at the uncore,
  // where it runs beside the LLC with the physical address available. At the
  // core, predict() runs at add_load_queue() before translation, so the
  // physical address is still 0 there — forbid it (mirror XPT).
  if (knob::ocp_perc_use_physical_address &&
      !knob::offchip_pred_location.compare("core")) {
    cerr << "[PERC] ERROR: ocp_perc_use_physical_address=true, but the "
            "perceptron OCP is placed inside the core, where the physical "
            "address is not available at prediction time (add_load_queue)."
         << endl
         << "[PERC] Set ocp_perc_use_physical_address=false, or move it beside "
            "the LLC (offchip_pred_location=uncore)."
         << endl;
    assert(false && "perc core-side cannot use physical address");
  }

  perc_pred = new perceptron_pred_t(
      knob::ocp_perc_activated_features, knob::ocp_perc_weight_array_sizes,
      knob::ocp_perc_feature_hash_types, knob::ocp_perc_activation_threshold,
      knob::ocp_perc_max_weight, knob::ocp_perc_min_weight,
      knob::ocp_perc_pos_weight_delta, knob::ocp_perc_neg_weight_delta,
      knob::ocp_perc_pos_train_thresh, knob::ocp_perc_neg_train_thresh);
  perc_pred->set_cpu(cpu);

  // init page buffer
  for (uint32_t index = 0; index < knob::ocp_perc_page_buf_sets; ++index) {
    deque<ocp_perc_page_buf_entry_t *> d;
    d.clear();
    m_page_buffer.push_back(d);
  }
  true_pos    = 0;
  false_pos   = 0;
  false_neg   = 0;
  true_neg    = 0;
  train_count = 0;

  reset_stats();
}

OffchipPredPerc::~OffchipPredPerc() {}

// Shared prediction logic. `info` carries the already-extracted features (built
// core- or uncore-side); everything below is placement-agnostic. The allocated
// feature state (info + perceptron weight sum) is handed back via `feature` so
// the caller can stash it where training will later find it (LSQ_ENTRY core-
// side, PACKET uncore-side).
bool OffchipPredPerc::predict_helper(state_info_t        *info,
                                     ocp_perc_feature_t *&feature)
{
  float perc_weight_sum = 0.0;
  bool  prediction      = false;

  // get prediction
  perc_pred->predict(info, prediction, perc_weight_sum);

  // save all necessary data that would
  // later be required for training
  feature                  = new ocp_perc_feature_t();
  feature->info            = info;
  feature->perc_weight_sum = perc_weight_sum;

  stats.predict.called++;
  stats.predict.outcome[prediction]++;

  return prediction;
}

// Shared training logic. The prediction (`went_offchip_pred`), the resolved
// outcome (`went_offchip`), and the feature state saved at predict time are
// supplied by the caller; the source (LSQ_ENTRY vs PACKET) is irrelevant here.
void OffchipPredPerc::train_helper(ocp_perc_feature_t *feature,
                                   bool went_offchip_pred, bool went_offchip)
{
  train_count++;

  // keep track of true/false positives/negatives
  if (went_offchip_pred && went_offchip) {
    true_pos++;
  } else if (went_offchip_pred && !went_offchip) {
    false_pos++;
  } else if (!went_offchip_pred && went_offchip) {
    false_neg++;
  } else if (!went_offchip_pred && !went_offchip) {
    true_neg++;
  }

  // check if an update to activation threshold is needed
  if (knob::ocp_perc_enable_dynamic_act_thresh &&
      train_count % knob::ocp_perc_update_act_thresh_epoch == 0) {
    check_and_update_act_thresh();
  }

  // retreive all necessary data that were
  // used before for prediction making
  state_info_t *info            = feature->info;
  float         perc_weight_sum = feature->perc_weight_sum;

  // record the resolved outcome in the page buffer
  // (PageOffchipCount / PageMissRatio)
  record_page_outcome(info->page, went_offchip);

  // train perceptron
  perc_pred->train(info, perc_weight_sum, went_offchip_pred, went_offchip);

  stats.train.called++;
}

uint32_t OffchipPredPerc::get_set(uint64_t page)
{
  uint32_t hash = HashZoo::fnv1a64(page);
  return hash % knob::ocp_perc_page_buf_sets;
}

void OffchipPredPerc::get_data_flow_signatures(state_info_t *info,
                                               uint64_t      addr)
{
  // address decompositions
  info->addr   = addr;
  info->page   = addr >> LOG2_PAGE_SIZE;
  info->offset = (addr >> LOG2_BLOCK_SIZE) &
                 ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
  info->cl_offset       = addr & ((1ull << LOG2_BLOCK_SIZE) - 1);
  info->cl_word_offset  = info->cl_offset >> 2;
  info->cl_dword_offset = info->cl_offset >> 4;

  // page-buffer state
  uint64_t page   = info->page;
  uint32_t offset = (uint32_t)info->offset;

  stats.page_buf.called++;
  unique_pages.insert(page);

  ocp_perc_page_buf_entry_t *entry = NULL;
  uint32_t                   set   = get_set(page);
  auto                       it    = find_if(
      m_page_buffer[set].begin(), m_page_buffer[set].end(),
      [page](ocp_perc_page_buf_entry_t *entry) { return entry->page == page; });

  if (it != m_page_buffer[set].end())  // page hit
  {
    entry              = (*it);
    info->first_access = !entry->bmp_access.test(offset);
    entry->bmp_access.set(offset);
    // spatial pattern INCLUDING the current access
    info->page_spatial_footprint = BitmapHelper::value(entry->bmp_access);
    entry->age                   = 0;
    // the feature reads the count of PRIOR accesses to this page
    info->page_reuse_count = entry->reuse_count;
    entry->reuse_count++;
    // outcomes observed so far (incremented at train, read here)
    info->page_offchip_count = entry->offchip_count;
    info->page_trained_count = entry->trained_count;
    m_page_buffer[set].erase(it);
    m_page_buffer[set].push_back(entry);
    stats.page_buf.hit++;
  } else {
    if (m_page_buffer[set].size() >= knob::ocp_perc_page_buf_assoc) {
      entry = m_page_buffer[set].front();
      m_page_buffer[set].pop_front();
      stats.page_buf.eviction++;
      delete entry;
    }

    entry       = new ocp_perc_page_buf_entry_t();
    entry->page = page;
    entry->bmp_access.set(offset);
    entry->age = 0;
    // first touch while resident: zero prior accesses, zero outcomes;
    // the footprint holds just the current access
    info->page_reuse_count       = 0;
    entry->reuse_count           = 1;
    info->page_offchip_count     = 0;
    info->page_trained_count     = 0;
    info->page_spatial_footprint = BitmapHelper::value(entry->bmp_access);
    m_page_buffer[set].push_back(entry);
    info->first_access = true;
    stats.page_buf.insertion++;
  }
}

// Train-side page-buffer update for PageOffchipCount / PageMissRatio: the
// outcome is known only at train time. Deliberately no LRU promotion and no
// insertion on miss (the page may have been evicted since predict) -- the
// train path must not perturb the predict-path state (eviction order drives
// first_access).
void OffchipPredPerc::record_page_outcome(uint64_t page, bool went_offchip)
{
  uint32_t set = get_set(page);
  auto     it  = find_if(
      m_page_buffer[set].begin(), m_page_buffer[set].end(),
      [page](ocp_perc_page_buf_entry_t *entry) { return entry->page == page; });
  if (it != m_page_buffer[set].end()) {
    (*it)->trained_count++;
    if (went_offchip) {
      (*it)->offchip_count++;
    }
  }
}

void OffchipPredPerc::get_control_flow_signatures(state_info_t *info,
                                                  uint64_t      curr_pc,
                                                  int           rob_index)
{
  info->pc = curr_pc;

  // signature from last N load PCs
  if (last_n_load_pcs.size() >= knob::ocp_perc_last_n_load_pcs) {
    last_n_load_pcs.pop_front();
  }
  last_n_load_pcs.push_back(curr_pc);

  info->last_n_load_pc_sig = 0;
  for (uint32_t index = 0; index < last_n_load_pcs.size(); ++index) {
    info->last_n_load_pc_sig <<= 1;
    info->last_n_load_pc_sig ^= last_n_load_pcs[index];
  }

  // signature from last N instruction PCs
  deque<uint64_t> last_n_pcs;
  int             prior = rob_index;
  for (int i = 0; i < (int)knob::ocp_perc_last_n_pcs - 1; ++i) {
    last_n_pcs.push_front(ooo_cpu[cpu].ROB.entry[prior].ip);
    prior--;
    if (prior < 0) {
      prior = ooo_cpu[cpu].ROB.SIZE - 1;
    }
  }

  info->last_n_pc_sig = 0;
  for (uint32_t index = 0; index < last_n_pcs.size(); ++index) {
    info->last_n_pc_sig <<= 1;
    info->last_n_pc_sig ^= last_n_pcs[index];
  }
}

string
OffchipPredPerc::print_activated_features(vector<int32_t> activated_features)
{
  std::stringstream ss;
  for (uint32_t feature = 0; feature < activated_features.size(); ++feature) {
    if (feature) {
      ss << ",";
    }
    ss << perc::feature_names[activated_features[feature]];
  }
  return ss.str();
}

void OffchipPredPerc::check_and_update_act_thresh()
{
  stats.act_thresh_update.called++;
  float precision = (float)true_pos / (true_pos + false_pos);

  // if the DRAM bandwidth consumption is high AND the precision of the
  // predictor is low, then try to improve the precision by increasing
  // activation threshold
  if (dram_bw >= knob::ocp_perc_high_critical_dram_bw_level &&
      precision <= knob::ocp_perc_poor_precision_thresh) {
    stats.act_thresh_update.increment++;
    float old_thresh = perc_pred->get_activation_threshold();
    float new_thresh =
        min((old_thresh + knob::ocp_perc_act_thresh_update_gradient),
            knob::ocp_perc_max_activation_threshold);
    perc_pred->set_activation_threshold(new_thresh);

    MYLOG(warmup_complete[cpu],
          "incr_act dram_bw: %d true_pos: %ld false_pos: %ld false_neg: %ld "
          "true_neg: %ld precision: %f recall: %f old_act_thresh: %f "
          "new_act_thresh: %f",
          +dram_bw, true_pos, false_pos, false_neg, true_neg, precision, recall,
          old_thresh, new_thresh);

    // stats collect
    if (new_thresh < stats.act_thresh_update.min_observed_thresh) {
      stats.act_thresh_update.min_observed_thresh = new_thresh;
    }
    if (new_thresh > stats.act_thresh_update.max_observed_thresh) {
      stats.act_thresh_update.max_observed_thresh = new_thresh;
    }
  } else if (dram_bw <= knob::ocp_perc_low_critical_dram_bw_level) {
    stats.act_thresh_update.decrement++;
    float old_thresh = perc_pred->get_activation_threshold();
    float new_thresh =
        max((old_thresh - knob::ocp_perc_act_thresh_update_gradient),
            knob::ocp_perc_min_activation_threshold);
    perc_pred->set_activation_threshold(new_thresh);

    MYLOG(warmup_complete[cpu],
          "decr_act dram_bw: %d true_pos: %ld false_pos: %ld false_neg: %ld "
          "true_neg: %ld precision: %f recall: %f old_act_thresh: %f "
          "new_act_thresh: %f",
          +dram_bw, true_pos, false_pos, false_neg, true_neg, precision, recall,
          old_thresh, new_thresh);

    // stats collect
    if (new_thresh < stats.act_thresh_update.min_observed_thresh) {
      stats.act_thresh_update.min_observed_thresh = new_thresh;
    }
    if (new_thresh > stats.act_thresh_update.max_observed_thresh) {
      stats.act_thresh_update.max_observed_thresh = new_thresh;
    }
  } else {
    MYLOG(warmup_complete[cpu],
          "act_same dram_bw: %d true_pos: %ld false_pos: %ld false_neg: %ld "
          "true_neg: %ld precision: %f recall: %f",
          +dram_bw, true_pos, false_pos, false_neg, true_neg, precision,
          recall);
  }
}

//=============================================================================
// Core-side placement (offchip_pred_location == core): LSQ_ENTRY interface
//=============================================================================

bool OffchipPredPerc::predict(ooo_model_instr *arch_instr, uint32_t data_index,
                              LSQ_ENTRY *lq_entry)
{
  state_info_t       *info       = get_state(arch_instr, data_index, lq_entry);
  ocp_perc_feature_t *feature    = NULL;
  bool                prediction = predict_helper(info, feature);

  // save the feature state in the LQ entry for training at LQ release
  lq_entry->ocp_feature = feature;

  return prediction;
}

void OffchipPredPerc::train(ooo_model_instr *arch_instr, uint32_t data_index,
                            LSQ_ENTRY *lq_entry)
{
  // retreive the feature state saved in the LQ entry at predict time
  train_helper((ocp_perc_feature_t *)lq_entry->ocp_feature,
               lq_entry->went_offchip_pred, lq_entry->went_offchip);
}

state_info_t *OffchipPredPerc::get_state(ooo_model_instr *arch_instr,
                                         uint32_t         data_index,
                                         LSQ_ENTRY       *lq_entry)
{
  // address source per the knob; physical@core is asserted out in the
  // constructor, so this resolves to the virtual address at the core.
  uint64_t addr = knob::ocp_perc_use_physical_address
                      ? lq_entry->physical_address
                      : lq_entry->virtual_address;

  state_info_t *info = new state_info_t();
  info->data_index   = data_index;
  get_control_flow_signatures(info, lq_entry->ip, lq_entry->rob_index);
  get_data_flow_signatures(info, addr);

  return info;
}

//=============================================================================
// Uncore-side placement (LLC; offchip_pred_location == uncore): PACKET
// interface
//=============================================================================

bool OffchipPredPerc::predict(PACKET *packet)
{
  state_info_t       *info       = get_state(packet);
  ocp_perc_feature_t *feature    = NULL;
  bool                prediction = predict_helper(info, feature);

  // the feature state rides on the PACKET into the LLC RQ; the LLC trains on it
  // after the tag lookup resolves, then frees it.
  packet->ocp_feature = feature;

  return prediction;
}

void OffchipPredPerc::train(PACKET *packet)
{
  // Train fires exactly once, at RQ release, so every trained load must have
  // been predicted at the L2 miss => its feature is non-NULL. Assert rather
  // than silently skip: a NULL here means a load reached train without a paired
  // predict, and we want that to fail loudly instead of running wrongly.
  assert(packet->ocp_feature != NULL &&
         "uncore perc train: ocp_feature is NULL (unpaired predict/train)");
  // retreive the feature state carried on the PACKET from predict time
  train_helper((ocp_perc_feature_t *)packet->ocp_feature,
               packet->went_offchip_pred, packet->went_offchip);
}

// Uncore feature extraction: same feature set as the core get_state(), but
// sourced from the PACKET. The virtual address rides on packet->full_virt_addr
// (added so the uncore indexes the same virtual-address features); the PC,
// rob_index, and data_index ride on the packet too.
state_info_t *OffchipPredPerc::get_state(PACKET *packet)
{
  // address source per the knob: physical (packet->full_addr) at the uncore,
  // else the virtual address carried via packet->full_virt_addr.
  uint64_t addr = knob::ocp_perc_use_physical_address ? packet->full_addr
                                                      : packet->full_virt_addr;

  state_info_t *info = new state_info_t();
  info->data_index   = packet->data_index;
  get_control_flow_signatures(info, packet->ip, packet->rob_index);
  get_data_flow_signatures(info, addr);

  return info;
}
