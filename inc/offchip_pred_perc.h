#ifndef OFFCHIP_PRED_PERC_H
#define OFFCHIP_PRED_PERC_H

#include <unordered_set>
#include "offchip_pred_base.h"
#include "perc_pred.h"
#include "bitmap.h"

using namespace perc;

/* Feature set of OCP_Perc
 * Note: this feature set will be stored in LQ entry
 * and later used for training OCP */
class ocp_perc_feature_t : public ocp_base_feature_t
{
public:
  state_info_t *info;
  float         perc_weight_sum;

  ocp_perc_feature_t()
  {
    info            = NULL;
    perc_weight_sum = 0.0;
  }
  virtual ~ocp_perc_feature_t()  // MUST be virtual
  {
    if (info) {
      delete info;
    }
  }
};

class ocp_perc_page_buf_entry_t
{
public:
  uint64_t page;
  Bitmap   bmp_access;
  uint32_t age;

public:
  ocp_perc_page_buf_entry_t()
  {
    page = 0;
    bmp_access.reset();
    age = 0;
  }
};

class OffchipPredPerc : public OffchipPredBase
{
private:
  perceptron_pred_t                         *perc_pred;
  vector<deque<ocp_perc_page_buf_entry_t *>> m_page_buffer;
  deque<uint64_t>                            last_n_load_pcs;

  // counters to measure true/false positives/negatives
  uint64_t true_pos, false_pos, false_neg, true_neg;
  uint64_t train_count;

  // for measuring stats
  unordered_set<uint64_t> unique_pages;

  struct {
    struct {
      uint64_t called;
    } train;

    struct {
      uint64_t called;
      uint64_t outcome[2];
    } predict;

    struct {
      uint64_t called;
      uint64_t hit;
      uint64_t eviction;
      uint64_t insertion;
    } page_buf;

    struct {
      uint64_t called;
      uint64_t increment;
      uint64_t decrement;
      float    min_observed_thresh;
      float    max_observed_thresh;
    } act_thresh_update;

  } stats;

  // shared core/uncore logic, parameterized by the already-extracted features
  bool predict_helper(state_info_t *info, ocp_perc_feature_t *&feature);
  void train_helper(ocp_perc_feature_t *feature, bool went_offchip_pred,
                    bool went_offchip);

  // Feature extraction differs only by placement: core reads the LSQ_ENTRY,
  // uncore reads the PACKET (the virtual address rides on full_virt_addr).
  state_info_t *get_state(ooo_model_instr *arch_instr, uint32_t data_index,
                          LSQ_ENTRY *lq_entry);
  state_info_t *get_state(PACKET *packet);

  void     lookup_address(uint64_t addr, uint64_t page, uint32_t offset,
                          bool &first_access);
  uint32_t get_set(uint64_t page);
  void     get_control_flow_signatures(uint64_t curr_pc, int rob_index,
                                       uint64_t &last_n_load_pc_sig,
                                       uint64_t &last_n_pc_sig);

  string print_activated_features(vector<int32_t> activated_features);
  void   check_and_update_act_thresh();

public:
  OffchipPredPerc(uint32_t cpu, string _type, uint64_t _seed);
  ~OffchipPredPerc();

  void print_config();
  void dump_stats();
  void reset_stats();
  void train(ooo_model_instr *arch_instr, uint32_t data_index,
             LSQ_ENTRY *lq_entry);
  bool predict(ooo_model_instr *arch_instr, uint32_t data_index,
               LSQ_ENTRY *lq_entry);

  // Uncore (beside-LLC) path: operate on the PACKET (the physical address and
  // the virtual address via full_virt_addr are both available).
  void train(PACKET *packet);
  bool predict(PACKET *packet);
};

#endif /* OFFCHIP_PRED_PERC_H */
