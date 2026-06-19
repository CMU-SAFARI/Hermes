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

namespace knob
{
    extern string offchip_pred_type;
    extern bool   offchip_pred_mark_merged_load;
    extern string offchip_pred_location;
}

//=============================================================================
// Common: shared by the core path (O3_CPU) and the uncore path (LLC CACHE)
//=============================================================================

// Factory: build the predictor selected by offchip_pred_type and return it.
// `owner` is "core" or "LLC" (purely for logging + the LLC-support guard).
static OffchipPredBase* create_offchip_predictor(uint32_t cpu, string type, uint64_t seed, string owner)
{
    // single, greppable log line: "Adding Offchip predictor: <type> at <core/LLC>"
    cout << "Adding Offchip predictor: " << type << " at " << owner << endl;

    // LLC-side (uncore) support is added incrementally; only the types that have a
    // working uncore PACKET* path may live at the LLC. For now that is just "none".
    // Relax this as XPT/perc/etc. gain their uncore implementations.
    if(!owner.compare("LLC"))
        assert(!type.compare("none") && "LLC-side offchip predictor currently supports only type 'none'");

    if(!type.compare("none"))
            return new OffchipPredBase(cpu, type, seed);
    else if(!type.compare("basic"))
            return new OffchipPredBasic(cpu, type, seed);
    else if(!type.compare("random"))
            return new OffchipPredRandom(cpu, type, seed);
    else if(!type.compare("perc"))
            return new OffchipPredPerc(cpu, type, seed);
    else if(!type.compare("hmp-local"))
            return new OffchipPredHMPLocal(cpu, type, seed);
    else if(!type.compare("hmp-gshare"))
            return new OffchipPredHMPGshare(cpu, type, seed);
    else if(!type.compare("hmp-gskew"))
            return new OffchipPredHMPGskew(cpu, type, seed);
    else if(!type.compare("hmp-ensemble"))
            return new OffchipPredHMPEnsemble(cpu, type, seed);
    else if(!type.compare("ttp"))
            return new OffchipPredTTP(cpu, type, seed);
    else if(!type.compare("xpt"))
            return new OffchipPredXPT(cpu, type, seed);
    return NULL;
}

//=============================================================================
// O3_CPU: core-side placement (offchip_pred_location == core)
//=============================================================================

void O3_CPU::initialize_offchip_predictor(uint64_t seed)
{
    offchip_pred = create_offchip_predictor(cpu, knob::offchip_pred_type, seed, "core");
}

void O3_CPU::print_config_offchip_predictor()
{
    cout << "offchip_pred_type " << knob::offchip_pred_type << endl
         << "offchip_pred_mark_merged_load " << knob::offchip_pred_mark_merged_load << endl
         << "offchip_pred_location " << knob::offchip_pred_location << endl
         << endl;

    if(offchip_pred) offchip_pred->print_config();
}

void O3_CPU::dump_stats_offchip_predictor()
{
    float precision = (float)stats.offchip_pred.true_pos / (stats.offchip_pred.true_pos + stats.offchip_pred.false_pos),
          recall = (float)stats.offchip_pred.true_pos / (stats.offchip_pred.true_pos + stats.offchip_pred.false_neg);


    cout << "Core_" << cpu << "_offchip_pred_true_pos " << stats.offchip_pred.true_pos << endl
         << "Core_" << cpu << "_offchip_pred_false_pos " << stats.offchip_pred.false_pos << endl
         << "Core_" << cpu << "_offchip_pred_false_neg " << stats.offchip_pred.false_neg << endl
         << "Core_" << cpu << "_offchip_pred_precision " << precision*100 << endl
         << "Core_" << cpu << "_offchip_pred_recall " << recall*100 << endl
         << endl;

    if(offchip_pred) offchip_pred->dump_stats();
}

void O3_CPU::offchip_predictor_update_dram_bw(uint8_t dram_bw)
{
    if(offchip_pred) offchip_pred->update_dram_bw(dram_bw);
}

/* This function is called at every LLC eviction.
 * Cache-level prediction mechanisms that operate as tag-tracking mechanisms
 * need to track LLC evictions to make accurate off-chip predictions.
 * This function is written to track such evictions */
void O3_CPU::offchip_predictor_track_llc_eviction(uint32_t set, uint32_t way, uint64_t address)
{
	if(offchip_pred && !knob::offchip_pred_type.compare("ttp"))
	{
		OffchipPredTTP *ocp_lp = (OffchipPredTTP*) offchip_pred;
		ocp_lp->track_llc_eviction(address);
	}
}

//=============================================================================
// CACHE: uncore-side placement (offchip_pred_location == uncore; LLC-owned)
//=============================================================================

// The LLC owns a single predictor instance (per-core ooo_cpu[i].offchip_pred stay NULL).
void CACHE::initialize_offchip_predictor(uint64_t seed)
{
    offchip_pred = create_offchip_predictor(cpu, knob::offchip_pred_type, seed, "LLC");
}

void CACHE::print_config_offchip_predictor()
{
    cout << "offchip_pred_type " << knob::offchip_pred_type << endl
         << "offchip_pred_mark_merged_load " << knob::offchip_pred_mark_merged_load << endl
         << "offchip_pred_location " << knob::offchip_pred_location << endl
         << endl;

    if(offchip_pred) offchip_pred->print_config();
}

// Uncore train path: mirror of O3_CPU::offchip_pred_stats_and_train. The hit/miss
// outcome (packet->went_offchip) is set by the caller in CACHE::handle_read; the
// prediction (packet->went_offchip_pred) + feature state were set earlier by predict().
void CACHE::offchip_pred_stats_and_train(PACKET *packet)
{
    // per-core accuracy bookkeeping (same TP/FP/FN scheme as the core path)
    auto &ocp_stats = ooo_cpu[packet->cpu].stats.offchip_pred;
    if(packet->went_offchip && packet->went_offchip_pred)        ocp_stats.true_pos++;
    else if(!packet->went_offchip && packet->went_offchip_pred)  ocp_stats.false_pos++;
    else if(packet->went_offchip && !packet->went_offchip_pred)  ocp_stats.false_neg++;

    // train the LLC-owned predictor, then release the per-request feature state
    if(offchip_pred) offchip_pred->train(packet);
    if(packet->ocp_feature) { delete packet->ocp_feature; packet->ocp_feature = NULL; }
}
