#ifndef OFFCHIP_PRED_BLIND_H
#define OFFCHIP_PRED_BLIND_H

#include "offchip_pred_base.h"

// Zero-state, bandwidth-gated off-chip predictor. Keeps no page buffer,
// weights, history, or training state: it predicts off-chip iff the current
// DRAM bandwidth bucket is below a threshold. This is the "blind" baseline
// that isolates how much of DDRP's benefit comes purely from bandwidth gating,
// with no predictor state at all.
class OffchipPredBlind : public OffchipPredBase
{
public:
  OffchipPredBlind(uint32_t _cpu, string _type, uint64_t _seed)
      : OffchipPredBase(_cpu, _type, _seed)
  {
  }
  ~OffchipPredBlind() {}

  // Uncore (beside-LLC) path: the only override. Inherits base no-op train.
  bool predict(PACKET *packet);
};

#endif /* OFFCHIP_PRED_BLIND_H */
