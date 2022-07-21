#include <iostream>
#include <math.h>
#include <string.h>
#include "defs.h"
#include "mp3b_helper.h"

// features for original MICRO 2016 paper
// static feature_spec perceptron_specs[] = {
// { F_PC, 15, 0, 8, 0, 0 },
// { F_PC, 15, 1, 9, 1, 0 },
// { F_PC, 15, 2, 10, 2, 0 },
// { F_PC, 15, 3, 11, 3, 0 },
// { F_TAG, 15, 25, 33, 0, 0 },
// { F_TAG, 15, 28, 36, 0, 0 },
// };

// features for the various configurations
// this is used in single-core
static feature_spec default_single_1_specs[] = {
{ F_OFF, 15, 1, 6, 0, 1 },
{ F_PC, 7, 14, 43, 11, 0 },
{ F_PC, 16, 3, 11, 16, 1 },
{ F_INS, 16, 0, 0, 0, 1 },
{ F_OFF, 10, 0, 6, 0, 1 },
{ F_PC, 10, 1, 53, 10, 0 },
{ F_BIAS, 16, 0, 0, 0, 0 },
{ F_INS, 8, 0, 0, 0, 1 },
{ F_PC, 17, 6, 20, 0, 1 },
{ F_BURST, 6, 11, 22, 9, 0 },
{ F_LM, 9, 0, 0, 0, 0 },
{ F_PC, 17, 6, 20, 0, 1 },
{ F_INS, 16, 2, 47, 2, 0 },
{ F_INS, 17, 0, 0, 0, 1 },
{ F_PC, 16, 8, 16, 5, 0 },
{ F_PC, 17, 6, 20, 14, 1 },
};

// this is used in multi-core
static feature_spec default_multi_3_specs[] = {
{ F_BIAS, 1, 0, 0, 0, 0 },
{ F_PC, 16, 9, 25, 9, 1 },
{ F_INS, 8, 4, 8, 7, 2 },
{ F_PC, 6, 9, 28, 12, 1 },
{ F_OFF, 14, 1, 4, 0, 3 },
{ F_LM, 7, 7, 51, 3, 1 },
{ F_PC, 10, 1, 54, 13, 3 },
{ F_PC, 10, 3, 32, 5, 1 },
{ F_PC, 14, 5, 24, 0, 1 },
{ F_OFF, 13, 4, 4, 0, 2 },
{ F_TAG, 8, 4, 47, 11, 2 },
{ F_TAG, 2, 24, 32, 0, 1 },
{ F_PC, 12, 10, 30, 0, 1 },
{ F_PC, 12, 9, 28, 0, 2 },
{ F_PC, 12, 5, 31, 2, 2 },
{ F_TAG, 8, 10, 8, 7, 1 },
// # 23.392479
};

// extra configurations
// static feature_spec default_single_2_specs[] = {
// { F_INS, 15, 7, 55, 1, 0 },
// { F_INS, 16, 0, 0, 0, 1 },
// { F_INS, 6, 13, 38, 0, 1 },
// { F_OFF, 14, 0, 7, 0, 2 },
// { F_BIAS, 17, 8, 23, 10, 3 },
// { F_BURST, 8, 1, 11, 13, 2 },
// { F_PC, 6, 5, 48, 0, 3 },
// { F_LM, 15, 16, 44, 0, 2 },
// { F_TAG, 17, 1, 32, 14, 2 },
// { F_PC, 17, 6, 20, 0, 1 },
// { F_PC, 6, 4, 11, 2, 2 },
// { F_BIAS, 13, 8, 67, 7, 2 },
// { F_OFF, 8, 1, 6, 0, 2 },
// { F_PC, 6, 5, 77, 4, 1 },
// { F_TAG, 11, 8, 19, 7, 0 },
// { F_TAG, 16, 8, 16, 0, 0 },
// // # 3.511360
// };
// static feature_spec default_multi_4_specs[] = {
// { F_LM, 9, 5, 17, 4, 0 },
// { F_PC, 8, 6, 8, 14, 3 },
// { F_BIAS, 13, 9, 40, 10, 3 },
// { F_OFF, 8, 2, 2, 0, 2 },
// { F_TAG, 16, 3, 14, 11, 2 },
// { F_BURST, 16, 14, 28, 9, 2 },
// { F_INS, 10, 4, 14, 3, 2 },
// { F_PC, 14, 3, 18, 10, 2 },
// { F_INS, 6, 11, 18, 9, 0 },
// { F_PC, 17, 1, 14, 5, 0 },
// { F_OFF, 11, 2, 5, 0, 0 },
// { F_OFF, 15, 0, 7, 0, 3 },
// { F_TAG, 9, 2, 13, 7, 2 },
// { F_TAG, 15, 4, 34, 3, 2 },
// { F_OFF, 10, 0, 6, 0, 1 },
// { F_PC, 11, 7, 23, 0, 2 },
// // # 9.734541
// };

// set the parameters based on configuration number
void mp3b_params::set_parameters(uint32_t num_cpus) 
{
	if(num_cpus == 1)
	{
		dan_promotion_threshold = 82;
		dan_init_weight = 1;
		dan_dt1 = 56;
		dan_dt2 = 256;
		dan_leaders = 34;
		dan_ignore_prefetch = 1;
		dan_use_plru = 0;
		dan_use_rrip = 0;
		dan_bypass_threshold = 48;
		dan_record_types = 27;
		dan_sampler_assoc = 18;
		dan_predictor_index_bits = 8;
		dan_predictor_tables = 16;
		dan_counter_width = 6;
		dan_threshold = 128;
		dan_theta = 109;
		dan_theta2 = 135;
		dan_sampler_tag_bits = 16;
		dan_samplers = 80;
		specs = default_single_1_specs;
		plv[0][0] = -15;
		plv[0][1] = 0;
		plv[1][0] = 35;
		plv[1][1] = 12;
		plv[2][0] = 44;
		plv[2][1] = 15;
	}
	else if(num_cpus >= 1)
	{
		dan_promotion_threshold = 256;
		dan_rrip_place_position = 2,
		dan_init_weight = 1;
		dan_dt1 = 27;
		dan_dt2 = 229;
		dan_leaders = 34;
		dan_ignore_prefetch = 1;
		dan_use_plru = 0;
		dan_use_rrip = 1;
		dan_bypass_threshold = -3;
		dan_record_types = 27;
		dan_sampler_assoc = 18;
		dan_predictor_index_bits = 8;
		dan_predictor_tables = 16;
		dan_counter_width = 6;
		dan_threshold = 0;
		dan_theta = 0;
		dan_theta2 = 0;
		dan_sampler_tag_bits = 16;
		dan_samplers = 308;
		specs = default_multi_3_specs;
		plv[0][0] = -230;
		plv[0][1] = 1;
		plv[1][0] = 12;
		plv[1][1] = 2;
		plv[2][0] = 22;
		plv[2][1] = 3;
	}
	else // should not reach here
	{
		cout << "[mp3b_helper] num_cpus is less than 1" << endl;
		assert(0);
	}
}

// constructor for a sampler set
sdbp_sampler_set::sdbp_sampler_set (mp3b_params *_params) : params(_params)
{
	// allocate some sampler entries
	blocks = new sdbp_sampler_entry[params->dan_sampler_assoc];

	// initialize the LRU replacement algorithm for these entries
	for (int i=0; i<params->dan_sampler_assoc; i++)
		blocks[i].lru_stack_position = i;
}

// access the sampler with an LLC tag
void sdbp_sampler::access (uint32_t tid, int set, int real_set, uint64_t tag, uint64_t PC, int accessType, uint64_t paddr) 
{
	// get a pointer to this set's sampler entries
	sdbp_sampler_entry *blocks = &sets[set].blocks[0];

	// get a partial tag to search for
	unsigned int partial_tag = tag & ((1<<params->dan_sampler_tag_bits)-1);

	// this will be the way of the sampler entry we end up hitting or replacing
	int i;

	// search for a matching tag
	// no valid bits; tags are initialized to 0, and if we accidentally
	// match a 0 that's OK because we don't need correctness
	for (i=0; i<params->dan_sampler_assoc; i++) if (blocks[i].tag == partial_tag) 
    {
		// we know this block is not dead; inform the predictor
		pred->block_is_dead (tid, &blocks[i], blocks[i].trace_buffer, false, blocks[i].conf, blocks[i].lru_stack_position);
		break;
	}

	// did we find a match?
	bool is_fill = false;

	if (i == params->dan_sampler_assoc) 
    {
		// find the LRU block
		if (i == params->dan_sampler_assoc) 
        {
			int j;
			for (j=0; j<params->dan_sampler_assoc; j++)
				if (blocks[j].lru_stack_position == (unsigned int) (params->dan_sampler_assoc-1)) break;
			assert (j < params->dan_sampler_assoc);
			i = j;
		}

		// previous trace leads to block being dead; inform the predictor
		pred->block_is_dead (tid, &blocks[i], blocks[i].trace_buffer, true, blocks[i].conf, params->dan_sampler_assoc);

		// reminds us to fill the block later (after we're done
		// using the current victim's metadata)
		is_fill = true;
	}

	// now the replaced or hit entry should be moved to the MRU position
	unsigned int position = blocks[i].lru_stack_position;
	for(int way=0; way<params->dan_sampler_assoc; way++) 
    {
		if (blocks[way].lru_stack_position < position) 
        {
			blocks[way].lru_stack_position++;
			// inform the predictor that this block has reached
			// this position
			pred->block_is_dead (tid, &blocks[way], blocks[way].trace_buffer, true, blocks[way].conf, blocks[way].lru_stack_position);
		}
	}
	blocks[i].lru_stack_position = 0;

	if (is_fill) 
    {
		// fill the victim block
		blocks[i].tag = partial_tag;
	}

	// record the trace
	make_trace (tid, pred, real_set, PC, tag, accessType, position == 0, is_fill, lastmiss_bits[real_set], paddr & 63);
	memcpy (blocks[i].trace_buffer, trace_buffer, (MAX_PATH_LENGTH+1) * sizeof (unsigned int));

	// get the next prediction for this entry
	blocks[i].conf = pred->get_prediction (tid, -1);
}

// make a trace from a PC (just extract some bits)
void sdbp_sampler::make_trace(uint32_t tid, perceptron_predictor *pred, uint32_t setIndex, uint64_t PC, uint32_t tag, int accessType, bool burst, bool insertion, bool lastmiss, unsigned int offset)
{
	for (int i=0; i<params->dan_predictor_tables; i++) {
		feature_spec *f = &params->specs[i];
		int begin_shift = f->begin;
		int end_mask = (1<<(f->end - begin_shift))-1; // TODO: make this a hash instead of a simple bit extract
		switch (f->type) {
		case F_PC:
			if (f->which == 0)
				trace_buffer[i] = (PC >> begin_shift) & end_mask;
			else
				trace_buffer[i] = (addresses[tid][f->which-1] >> begin_shift) & end_mask;
			break;
		case F_TAG:
			trace_buffer[i] = (((tag << lognsets6) | (setIndex << 6)) >> begin_shift) & end_mask;
			break;
		case F_BIAS:
			trace_buffer[i] = 0;
			break;
		case F_BURST:
			trace_buffer[i] = burst;
			break;
		case F_INS:
			trace_buffer[i] = insertion;
			break;
		case F_LM:
			trace_buffer[i] = lastmiss;
			break;
		case F_OFF:
			trace_buffer[i] = (offset >> begin_shift) & end_mask;
			break;
		}
		if (f->xorpc & 1) trace_buffer[i] ^= PC;
		if (f->xorpc & 2) trace_buffer[i] ^= 2 * (accessType == PREFETCH);
	}
}

sdbp_sampler::sdbp_sampler (mp3b_params *_params, int nsets, int assoc) : params(_params)
{
	if (NUM_CPUS == 1) assert (!params->dan_use_rrip);
	if (NUM_CPUS == 4) assert (params->dan_use_rrip);

	leaders1 = params->dan_leaders * 1;
	leaders2 = params->dan_leaders * 2;

    lognsets6 = log2(LLC_SET) + 6;

    // initialize lastmiss feature
	lastmiss_bits = new bool[LLC_SET];
	memset (lastmiss_bits, 0, LLC_SET);

    // per-core arrays of recent PCs (we count the bits later when we know the max PC width)
	addresses = new unsigned int*[NUM_CPUS];
	for (unsigned int i=0; i<NUM_CPUS; i++) {
		addresses[i] = new unsigned int[MAX_PATH_LENGTH];
		memset (addresses[i], 0, sizeof (int) * MAX_PATH_LENGTH);
	}

	// here, we figure out the total number of bits used by the various
	// structures etc.  along the way we will figure out how many
	// sampler sets we have room for
	params->dan_predictor_table_entries = 1 << params->dan_predictor_index_bits;
	nsampler_sets = params->dan_samplers;

	if (nsampler_sets > nsets) 
    {
		nsampler_sets = nsets;
		fprintf (stderr, "warning: number of sampler sets exceeds number of real sets, setting nsampler_sets to %d\n", nsampler_sets);
		fflush (stderr);
	}

	// compute the maximum saturating counter value; predictor constructor
	// needs this so we do it here
	params->dan_counter_max = (1 << (params->dan_counter_width-1)) -1;
	params->dan_counter_min = -(1 << (params->dan_counter_width-1));

	// make a predictor
	pred = new perceptron_predictor (params, this);

	// we should have at least one sampler set
	assert (nsampler_sets >= 0);

	// make the sampler sets
	sets = new sdbp_sampler_set [nsampler_sets];
    for(int index = 0; index < nsampler_sets; ++index) sets[index] = sdbp_sampler_set(params);
}

// constructor for the predictor
perceptron_predictor::perceptron_predictor (mp3b_params *_params, sdbp_sampler *samp) : params(_params), parent(samp)
{
	// make the tables
	tables = new int* [params->dan_predictor_tables];
	table_sizes = new int[params->dan_predictor_tables];

	// initialize each table
	for (int i=0; i<params->dan_predictor_tables; i++) 
    {
		int table_entries;
		switch (params->specs[i].type)
        {
		// 1 bit features
		case F_BIAS:
		case F_BURST:
		case F_LM:
		case F_INS:
			if (params->specs[i].xorpc == 0) table_entries = 2; else if (params->specs[i].xorpc == 2) table_entries = 4; else table_entries = params->dan_predictor_table_entries;
			break;
		case F_OFF:
			if (params->specs[i].xorpc == false) table_entries = 1<<(params->specs[i].end-params->specs[i].begin); else table_entries = params->dan_predictor_table_entries;
			break;
		default:
			table_entries = params->dan_predictor_table_entries;
		}
		table_sizes[i] = table_entries;
		tables[i] = new int[table_entries];
		for (int j=0; j<table_entries; j++) tables[i][j] = params->dan_init_weight;
		total_bits += 6 * table_entries;
	}
}

// inform the predictor that a block is either dead or not dead
// NOTE: the trace_buffer parameter here is from the block in the sampled
// set, not the global trace_buffer variable. yes, it is a hack.
void perceptron_predictor::block_is_dead (uint32_t tid, sdbp_sampler_entry *block, unsigned int *trace_buffer, bool d, int conf, int pos) 
{
	bool prediction = conf >= params->dan_threshold;
	bool correct = prediction == d;

	// perceptron learning rule: don't train if the prediction is
	// correct and the confidence is greater than some theta
	bool do_train = false;
	if (conf < 0) {
		if (conf > -params->dan_theta2) do_train = true;
	} else {
		if (conf < params->dan_theta) do_train = true;
	}
	if (!correct) do_train = true;
	if (!do_train) return;

	for (int i=0; i<params->dan_predictor_tables; i++) 
    {
		// for a "dead" block, only train wrt the associativity
		// for this feature
		if (d) if (params->specs[i].assoc != pos) continue;

		// for a "live" block, only train if it would have been a hit not a placement
		if (!d) if (params->specs[i].assoc <= pos) continue;

		// ...get a pointer to the corresponding entry in that table

		//int *c = &tables[i][trace_buffer[i] & ((1<<params->dan_predictor_index_bits)-1)];
		int *c = &tables[i][trace_buffer[i] % table_sizes[i]];

		// if the block is dead, increment the counter
		if (d) 
        {
			if (*c < params->dan_counter_max) (*c)++;
		} else 
        {
			if (*c > params->dan_counter_min) (*c)--;
		}
	}
}

// get a prediction for a given trace
// the trace is in trace_buffer[0..MAX_PATH_LENGTH]
int perceptron_predictor::get_prediction (uint32_t tid, int set) 
{
	// start the confidence sum as 0
	int conf = 0;

	// for each table...
	for (int i=0; i<params->dan_predictor_tables; i++) 
    {
		// ...get the counter value for that table...
		//int val = tables[i][trace_buffer[i] & ((1<<params->dan_predictor_index_bits)-1)];
		int val = tables[i][parent->trace_buffer[i] % table_sizes[i]];

		// and add it to the running total
		conf += val;
	}

	// if the counter is at least the threshold, the block is predicted dead

	// keep stored confidence to 9 bits
	if (conf > 255) conf = 255;
	if (conf < -256) conf = -256;
	return conf;
}