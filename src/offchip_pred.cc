#include <iostream>
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

namespace knob
{
    extern string offchip_pred_type;
    extern bool   offchip_pred_mark_merged_load;
}

void O3_CPU::initialize_offchip_predictor(uint64_t seed)
{
    if(!knob::offchip_pred_type.compare("none"))
    {
            cout << "Adding Offchip predictor: none" << endl;
            offchip_pred = (OffchipPredBase*) new OffchipPredBase(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("basic"))
    {
            cout << "Adding Offchip predictor: basic" << endl;
            offchip_pred = (OffchipPredBasic*) new OffchipPredBasic(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("random"))
    {
            cout << "Adding Offchip predictor: random" << endl;
            offchip_pred = (OffchipPredRandom*) new OffchipPredRandom(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("perc"))
    {
            cout << "Adding Offchip predictor: perceptron-based" << endl;
            offchip_pred = (OffchipPredPerc*) new OffchipPredPerc(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("hmp-local"))
    {
            cout << "Adding Offchip predictor: HMP-local" << endl;
            offchip_pred = (OffchipPredHMPLocal*) new OffchipPredHMPLocal(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("hmp-gshare"))
    {
            cout << "Adding Offchip predictor: HMP-Gshare" << endl;
            offchip_pred = (OffchipPredHMPGshare*) new OffchipPredHMPGshare(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("hmp-gskew"))
    {
            cout << "Adding Offchip predictor: HMP-Gskew" << endl;
            offchip_pred = (OffchipPredHMPGskew*) new OffchipPredHMPGskew(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("hmp-ensemble"))
    {
            cout << "Adding Offchip predictor: HMP-Ensemble" << endl;
            offchip_pred = (OffchipPredHMPEnsemble*) new OffchipPredHMPEnsemble(cpu, knob::offchip_pred_type, seed);
    }
    else if(!knob::offchip_pred_type.compare("ttp"))
    {
            cout << "Adding Offchip predictor: Tag-Tracking based Predictor (TTP)" << endl;
            offchip_pred = (OffchipPredTTP*) new OffchipPredTTP(cpu, knob::offchip_pred_type, seed);
    }
}

void O3_CPU::print_config_offchip_predictor()
{
    cout << "offchip_pred_type " << knob::offchip_pred_type << endl
         << "offchip_pred_mark_merged_load " << knob::offchip_pred_mark_merged_load << endl
         << endl;

    offchip_pred->print_config();
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

    offchip_pred->dump_stats();
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
	if(!knob::offchip_pred_type.compare("ttp"))
	{
		OffchipPredTTP *ocp_lp = (OffchipPredTTP*) offchip_pred;
		ocp_lp->track_llc_eviction(address);
	}
}