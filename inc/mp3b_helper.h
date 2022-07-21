#ifndef MP3B_HELPER_H
#define MP3B_HELPER_H

#include <cassert>
#include <cstddef>
#include <cstdint>
using namespace std;


// feature types
#define F_PC    0       // PC xor which PC (unless which = 0, then it's just PC)
#define F_TAG   1       // address
#define F_BIAS  2       // 0
#define F_BURST 3
#define F_INS	4	    // this is an insertion
#define F_LM	5
#define F_OFF	6

// a feature specifier
struct feature_spec 
{
	int     type;   // type of feature
	int     assoc;  // beyond which a block is considered evicted (dead)
	int     begin;  // beginning bit
	int     end;    // ending bit
	int     which;  // which (for PC)
	int     xorpc;  // xorpc & 1 => XOR with PC, xorpc & 2 => XOR with PREFETCH
};

// set from get_config_number()
#define MAX_PATH_LENGTH 16

// maximum number of specifiers to read
#define MAX_SPECS       (MAX_PATH_LENGTH+1)

// tree-based PseudoLRU operations
#define PLRU_LEFT(i)    ((i)*2+2)
#define PLRU_RIGHT(i)   ((i)*2+1)
#define SETBIT(z,k) ((z)|=(1<<(k)))
#define GETBIT(z,k) (!!((z)&(1<<(k))))

class mp3b_params
{
	public:
		int
			dan_promotion_threshold = 256,
			dan_init_weight = 1,
			dan_dt1 = 55, dan_dt2 = 1024,
			dan_rrip_place_position = 0,
			dan_leaders = 34,
			dan_ignore_prefetch = 1, // don't predict hitting prefetches
			dan_use_plru = 0, // 0 means use MDPP, 1 means use PLRU
			dan_use_rrip = 0, // 1 means use RRIP instead of MDPP/PLRU
			dan_bypass_threshold = 1000,
			dan_record_types = 27, // mask for deciding which kinds of memory accesses to record in history
			// sampler associativity 
			dan_sampler_assoc = 18,
			// number of bits used to index predictor; determines number of
			// entries in prediction tables
			dan_predictor_index_bits = 8,
			// number of prediction tables
			dan_predictor_tables = 16, // default specs have 16 features
			// width of prediction saturating counters
			dan_counter_width = 6,
			// predictor must meet this threshold to predict a block is dead
			dan_threshold = 8,
			dan_theta2 = 210,
			dan_theta = 110,
			// number of partial tag bits kept per sampler entry
			dan_sampler_tag_bits = 16,
			dan_samplers = 80,
			// number of entries in prediction table; derived from # of index bits
			dan_predictor_table_entries,
			// maximum value of saturating counter; derived from counter width
			dan_counter_min,
			dan_counter_max;

		feature_spec *specs = NULL; // which features to use
		
		// placement vector (initialized elsewhere)
		int plv[3][2] = {
			{ 0, 0 },
			{ 0, 0 },
			{ 0, 0 },
		};

		mp3b_params(){}
		~mp3b_params(){}
		void set_parameters(uint32_t num_cpus);
};

// one sampler entry
struct sdbp_sampler_entry 
{
	unsigned int 	
		lru_stack_position,
		tag,

		// copy of the trace used for the most recent prediction
		trace_buffer[MAX_PATH_LENGTH+1];
		
	// confidence from most recent prediction
	int conf;

	// constructor
	sdbp_sampler_entry (void) 
	{
		lru_stack_position = 0;
		tag = 0;
	};
};

// one sampler set (just a pointer to the entries)
struct sdbp_sampler_set 
{
	mp3b_params *params;
	sdbp_sampler_entry *blocks;

	sdbp_sampler_set(){} // dummy constructor
	sdbp_sampler_set (mp3b_params *params);
};

// forward declaration
struct sdbp_sampler;

// the dead block predictor
struct perceptron_predictor 
{
	mp3b_params *params;
	sdbp_sampler *parent;
	int **tables; 	// tables of two-bit counters
	int *table_sizes; // size of each table (not counted against h/w budget)
	int total_bits = 0; // for size calculation

	perceptron_predictor (mp3b_params *params, sdbp_sampler *parent);
	int get_prediction (uint32_t tid, int set);
	void block_is_dead (uint32_t tid, sdbp_sampler_entry *, unsigned int *, bool, int, int);
	inline int get_total_bits() {return total_bits;}
};

// the sampler
struct sdbp_sampler 
{
	mp3b_params *params;
	sdbp_sampler_set *sets;
	int nsampler_sets;   // number of sampler sets
	perceptron_predictor *pred;
	bool *lastmiss_bits;	// for lastmiss feature
	unsigned int leaders1 = 32, leaders2 = 64; // boundaries of leader sets
	unsigned int trace_buffer[MAX_PATH_LENGTH+1]; // trace is built here before prediction
	unsigned int **addresses; // per-core array of recent PCs
	int lognsets6;


	sdbp_sampler(mp3b_params *params, int nsets, int assoc);
	void access(uint32_t tid, int set, int real_set, uint64_t tag, uint64_t PC, int, uint64_t);
	void make_trace(uint32_t tid, perceptron_predictor *pred, uint32_t setIndex, uint64_t PC, uint32_t tag, int accessType, bool burst, bool insertion, bool lastmiss, unsigned int offset);
};


#endif /* MP3B_HELPER_H */


