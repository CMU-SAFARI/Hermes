#include <stdlib.h>
#include "mp3b.h"

void MP3BRepl::print_config()
{
    cout << "mp3b.promotion_threshold " << params->dan_promotion_threshold << endl
	    << "mp3b.rrip_place_position " << params->dan_rrip_place_position << endl
	    << "mp3b.init_weight " << params->dan_init_weight << endl
	    << "mp3b.dt1 " << params->dan_dt1 << endl
	    << "mp3b.dt2 " << params->dan_dt2 << endl
	    << "mp3b.leaders " << params->dan_leaders << endl
	    << "mp3b.ignore_prefetch " << params->dan_ignore_prefetch << endl
	    << "mp3b.use_plru " << params->dan_use_plru << endl
	    << "mp3b.use_rrip " << params->dan_use_rrip << endl
	    << "mp3b.bypass_threshold " << params->dan_bypass_threshold << endl
	    << "mp3b.record_types " << params->dan_record_types << endl
	    << "mp3b.sampler_assoc " << params->dan_sampler_assoc << endl
	    << "mp3b.predictor_index_bits " << params->dan_predictor_index_bits << endl
	    << "mp3b.predictor_tables " << params->dan_predictor_tables << endl
	    << "mp3b.counter_width " << params->dan_counter_width << endl
	    << "mp3b.threshold " << params->dan_threshold << endl
	    << "mp3b.theta " << params->dan_theta << endl
	    << "mp3b.theta2 " << params->dan_theta2 << endl
	    << "mp3b.sampler_tag_bits " << params->dan_sampler_tag_bits << endl
	    << "mp3b.samplers " << params->dan_samplers << endl
	    << "mp3b.specs " << params->specs << endl
	    << "mp3b.plv[0][0] " << params->plv[0][0] << endl
	    << "mp3b.plv[0][1] " << params->plv[0][1] << endl
	    << "mp3b.plv[1][0] " << params->plv[1][0] << endl
	    << "mp3b.plv[1][1] " << params->plv[1][1] << endl
	    << "mp3b.plv[2][0] " << params->plv[2][0] << endl
	    << "mp3b.plv[2][1] " << params->plv[2][1] << endl
        << endl;
}

// initialize replacement state
void MP3BRepl::initialize_replacement() 
{
	// config = get_config_number ();
	// switch (config) {
	// 	case 1: case 2:
	// 		NUM_CPUS = 1;
	// 		LLC_SET = 2048;
	// 		break;
	// 	case 3: case 4:
	// 		NUM_CPUS = 4;
	// 		LLC_SET = 8192;
	// 		break;
	// 	case 5: case 6:
	// 		NUM_CPUS = 1;
	// 		LLC_SET = 8192;
	// 		break;
	// 	default: assert (0);
	// }
	// printf ("config %d, NUM_CPUS %d, LLC_SET %d\n", config, NUM_CPUS, LLC_SET);

	// set parameters
	params = new mp3b_params();
	params->set_parameters(NUM_CPUS);

	// this variable helps put physical addresses back together

	lognsets = log2(LLC_SET);
	// if (LLC_SET == 2048) {
	// 	lognsets = 11;
	// } else if (LLC_SET == 8192) {
	// 	lognsets = 13;
	// } else assert (0);

	// initialize replacement state
	if (NUM_CPUS > 1) 
	{
		// multi-core configurations use RRIP
		rrpv = new unsigned char*[LLC_SET];
		int rrpv_bits = 0;
		for (unsigned int i=0; i<LLC_SET; i++)
		{
			rrpv[i] = new unsigned char[LLC_WAY];
			memset (rrpv[i], 3, LLC_WAY);
			rrpv_bits += 2 * LLC_WAY;
		}
		total_bits += rrpv_bits;
	} 
	else if (NUM_CPUS == 1) 
	{
		// single-core configurations use MDPP/PLRU
		int bits = 0;
		plru_bits = new bool*[LLC_SET];
		for (unsigned int i=0; i<LLC_SET; i++) {
			plru_bits[i] = new bool[LLC_WAY-1];
			memset (plru_bits[0], 0, LLC_WAY-1);
			bits += LLC_WAY-1;
		}
		total_bits += bits;
	} 
	else assert (0);

	total_bits += LLC_SET;

	// initialize sampler
	samp = new sdbp_sampler (params, LLC_SET, LLC_WAY);

	// at this point, the specs are set; we can compute the sizes
	int trace_bits = 0;
	for (int i=0; i<params->dan_predictor_tables; i++) {
		switch (params->specs[i].type) {
		case F_BIAS:
		case F_BURST:
		case F_LM:
		case F_INS:
			if (params->specs[i].xorpc == 0 || params->specs[i].xorpc == 2) trace_bits += 1; else trace_bits += params->dan_predictor_index_bits;
			break;
		case F_OFF:
			if (params->specs[i].xorpc == 0 || params->specs[i].xorpc == 2) trace_bits += (params->specs[i].end-params->specs[i].begin); 
			else trace_bits += params->dan_predictor_index_bits;
			break;
		default:
			trace_bits += params->dan_predictor_index_bits;
			break;
		}
	}

	int sampler_set_bits = params->dan_sampler_assoc * (
		4 // lru stack position
	+	params->dan_sampler_tag_bits
	+	trace_bits		// bits for the trace stored in each sampler entry
	+ 	9 // confidence bits
	);

	total_bits += trace_bits;	// global trace buffer


	// figure out how many bits we need to store the global array of PCs
	int max_end = 0;
	for (int i=0; i<params->dan_predictor_tables; i++) {
		if (params->specs[i].type == F_PC) if (params->specs[i].end > max_end) max_end = params->specs[i].end;
	}
	total_bits += max_end * MAX_PATH_LENGTH * NUM_CPUS;
	//int total_bits_allowed = 8 * 32768 * NUM_CPUS;
	//int sampler_bits = total_bits_allowed - total_bits;
	total_bits += 10; // for psel
	total_bits += 512; // in case someone wants to be pedantic about something
	total_bits += params->dan_samplers * sampler_set_bits;

	total_bits += samp->pred->get_total_bits();
}

void MP3BRepl::update_replacement_state (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit) 
{
	if (hit && (type == WRITEBACK)) return;
	uint64_t tag = paddr / (LLC_SET * 64);
	UpdateSampler (set, tag, cpu, PC, way, hit, type, paddr);
}

uint32_t MP3BRepl::find_victim (uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type) 
{
	return Get_Sampler_Victim (cpu, set, current_set, LLC_WAY, PC, paddr, type);
}

void MP3BRepl::dump_stats()
{

}

// update the PseudoLRU replacement state, using the placement vector and
// predictor confidence on a placement, and promotion threshold on a hit
void MP3BRepl::update_plru_mdpp (int set, int32_t wayID, bool hit, int *vector, uint32_t accessType, bool really, int conf) {
	assert (!params->dan_use_rrip);
	assert (wayID < 16);
	bool P[LLC_WAY-1];
	memcpy (P, plru_bits[set], LLC_WAY-1);
	unsigned int idx = 0;
	unsigned int x;
	bool wasodd = false;
	unsigned int newidx = 0;
	assert (vector);
	if (!hit) {
		if (conf >= params->plv[2][0]) newidx = params->plv[2][1];
		else if (conf >= params->plv[1][0]) newidx = params->plv[1][1];
		else if (conf >= params->plv[0][0]) newidx = params->plv[0][1];
	} else {
		// get the current plru index
		// build the index starting from a leaf and going to the root
		x = wayID + LLC_WAY - 1;
		int i = 0;
		while (x) {
			wasodd = x & 1;
			x = (x - 1) >> 1;
			// FIXME: here we say 3 but it should be log2(assoc)-1
			if (P[x] == wasodd) SETBIT(idx,3-i);
			i++;
		}
		assert (i == 4);
		newidx = vector[idx];
	}

	// set the new plru index for this block

	wasodd = false;
	x = wayID + LLC_WAY - 1;
	int i = 0;
	bool changed = false;
	while (x) {
		wasodd = x & 1;
		x = (x - 1) >> 1;
		// FIXME: here we say 3 but it should be log2(assoc)-1
		bool oldbit = P[x];
		P[x] = wasodd == GETBIT(newidx,3-i);
		if (P[x] != oldbit) changed = true;
		i++;
	}
	was_burst = !changed;
	assert (i == 4);
	if (conf > params->dan_promotion_threshold) really = false;

	// we might not "really" want to update state if we are just
	// calling this function to see if we had a cache burst

	if (really) memcpy (plru_bits[set], P, LLC_WAY-1);
}

// get the PseudoLRU victim
int MP3BRepl::get_mdpp_plru_victim (int set) 
{
	// find the pseudo-lru block
	bool *P = plru_bits[set];

	int a = LLC_WAY - 1;
	unsigned int x = 0;
	int level = 0;
	while (a) {
		level++;
		if (P[x])
			x = PLRU_RIGHT(x);
		else
			x = PLRU_LEFT(x);
		a /= 2;
	}
	x -= (LLC_WAY-1);
	return x;
}

// update replacement policy
void MP3BRepl::UpdateSampler (uint32_t setIndex, uint64_t tag, uint32_t tid, uint64_t PC, int32_t way, bool hit, uint32_t accessType, uint64_t paddr)
{
	// don't need to update on a bypass
	if (way >= 16) return;

	// make distinct PCs for hitting/missing prefetches
	if (accessType == PREFETCH) PC ^= (0xdeadbeef + hit);

	// make distinct PCs for hitting/missing writebacks, and skip a
	// bunch of stuff
	if (accessType == WRITEBACK) 
	{
		PC ^= 0x7e57ab1e;
		goto stuff;
	}

	// ignore hitting prefetches
	if (params->dan_ignore_prefetch == 1) 
	{
		if ((accessType == PREFETCH) && hit) goto stuff;
	}

	// update up/down counter for set-dueling
	if (setIndex < samp->leaders1) 
	{
		if (!hit) if (psel < 1023) psel++;
	} else if (setIndex < samp->leaders2) {
		if (!hit) if (psel > -1023) psel--;
	}

	// another place where we can ignore hitting prefetches
	if (params->dan_ignore_prefetch == 2) 
	{
		if ((accessType == PREFETCH) && hit) goto stuff;
	}
	// determine if this is a sampler set
	{
		// multiply set index by specially crafted invertible
		// matrix to distribute sampled sets across cache
		static unsigned int le11[] = { 0x37f, 0x431, 0x71d, 0x25c, 0x719, 0x4d5, 0x4b6, 0x2ca, 0x26d, 0x64f, 0x46d };
		static unsigned int le13[] = { 0x5c5, 0xcc5, 0xb6b, 0x1bc5, 0x8b, 0x1782, 0x190, 0x15dd, 0x1af8, 0x75e, 0x4a1, 0xb4b, 0x1196 };
		unsigned int *le = (NUM_CPUS == 1) ? le11 : le13;
		int set = mm (setIndex, le);

		// if this is a sampler set, access it
		if (set >= 0 && set < samp->nsampler_sets)
			samp->access (tid, set, setIndex, tag, PC, accessType, paddr);

		// update default replacement policy (MDPP or PLRU)
		int *vector;
		static int vecmdpp[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 0, 1, 0, 0 };
		static int vecplru[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		vector = params->dan_use_plru ? vecplru : vecmdpp;

		// do a fake promotion to see if this was a burst so we can get a prediction.
		if (params->dan_use_rrip) {
			was_burst = rrpv[setIndex][way] == 0;
		} else {
			update_plru_mdpp (setIndex, way, hit, vector, accessType, false);
		}

		// make the trace
		samp->make_trace (tid, samp->pred, setIndex, PC, tag, accessType, was_burst, !hit, samp->lastmiss_bits[setIndex], paddr & 63);

		// get the next prediction for this block using that trace
		int conf = samp->pred->get_prediction (tid, setIndex);
		if (params->dan_use_rrip) 
		{
			// what is the current RRPV value for this block?
			int position = rrpv[setIndex][way];
			if (!hit) 
			{
				// on a placement, use the placement vector
				position = params->dan_rrip_place_position;
				if (conf >= params->plv[2][0]) position = params->plv[2][1];
				else if (conf >= params->plv[1][0]) position = params->plv[1][1];
				else if (conf >= params->plv[0][0]) position = params->plv[0][1];
			}
			else
			{
				// on a hit, use the promotion threshold
				if (conf < params->dan_promotion_threshold)
					position = 0;
			}

			// assign the new RRPV

			rrpv[setIndex][way] = position;
		}
		else
		{
			// MDPP/PLRU replacement
			update_plru_mdpp (setIndex, way, hit, vector, accessType, true, conf);
		}
	}
stuff:
	// update address path

	bool record = false;
	if (accessType == LOAD) if (params->dan_record_types & 1) record = true;
	if (accessType == RFO) if (params->dan_record_types & 2) record = true;
	if (accessType == WRITEBACK) if (params->dan_record_types & 8) record = true;
	if (accessType == PREFETCH) if (params->dan_record_types & 16) record = true;
	if (record) 
	{
		memmove (&samp->addresses[tid][1], &samp->addresses[tid][0], (MAX_PATH_LENGTH-1) * sizeof (unsigned int));
		samp->addresses[tid][0] = PC;
	}
	samp->lastmiss_bits[setIndex] = !hit;
}

int MP3BRepl::Get_Sampler_Victim( uint32_t tid, uint32_t setIndex, const BLOCK *current_set, uint32_t assoc, uint64_t PC, uint64_t paddr, uint32_t accessType)
{
	// select a victim using default pseudo LRU policy
	assert (setIndex < LLC_SET);
	int r;
	if (params->dan_use_rrip)
	{
startover:
		int lrus[LLC_WAY], n = 0;
		for (unsigned int wayID=0; wayID<assoc; wayID++)
		{
			if (rrpv[setIndex][wayID] == 3) lrus[n++] = wayID;
		}
		if (n) 
		{
			r = lrus[rand()%n];
		}
		else 
		{
			for (unsigned int wayID=0; wayID<assoc; wayID++) rrpv[setIndex][wayID]++;
			goto startover;
		}
	}
	else
	{
		r = get_mdpp_plru_victim (setIndex);
	}
	// we now have a victim, r; predict whether this block is
	// "dead on arrival" and bypass for non-writeback accesses

	if (accessType != WRITEBACK)
	{
		uint32_t tag = paddr / (LLC_SET * 64);
		samp->make_trace (tid, samp->pred, setIndex, PC, tag, accessType, false, true, samp->lastmiss_bits[setIndex], paddr & 63);
		int prediction;
		int conf = samp->pred->get_prediction (tid, setIndex);
		if (params->dan_dt2)
		{
			if (setIndex < samp->leaders1) 
				prediction = conf >= params->dan_dt1;
			else if (setIndex < samp->leaders2)
				prediction = conf >= params->dan_dt2;
			else
			{
				if (psel >= 0)
					prediction = conf >= params->dan_dt2;
				else
					prediction = conf >= params->dan_dt1;
			}
		}
		else
		{
			prediction = conf >= params->dan_bypass_threshold;
		}
	
		// if block is predicted dead, then it should bypass the cache
	
		if (prediction)
		{
			r = LLC_WAY; // means bypass
		}
	}

	// return the selected victim

	return r;
}

// multiply bit vector x by matrix m (for shuffling set indices to determine sampler sets)
unsigned int MP3BRepl::mm (unsigned int x, unsigned int m[])
{
    unsigned int r = 0;
    for (int i=0; i<lognsets; i++) 
	{
            r <<= 1;
            unsigned int d = x & m[i];
            r |= __builtin_parity (d);
    }
    return r;
}