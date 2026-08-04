#include "offchip_pred_blind.h"
#include "uncore.h"
#include "knobs.h"

// Zero-state prediction: fire iff the current DRAM bandwidth bucket is below
// the threshold. No predictor state is consulted or updated; the packet is
// intentionally unused.
bool OffchipPredBlind::predict(PACKET *packet)
{
  return (uint32_t)uncore.DRAM.bw < knob::blind_ddrp_bw_thresh;
}
