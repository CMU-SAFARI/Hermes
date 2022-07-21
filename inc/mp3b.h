/********************************************************************
 * Multiperspective Promotion, Placemement, and Bypass Policy
 * Author: Daniel Jimenez        
 * Official code from 2nd Cache Repl. Championship
 ********************************************************************
 */

#ifndef MP3B_H
#define MP3B_H

#include <string.h>
#include <stdlib.h>
#include "defs.h"
#include "cache_repl_base.h"
#include "mp3b_helper.h"

class MP3BRepl : public CacheReplBase
{
	private:
		mp3b_params *params; // parameters
		sdbp_sampler *samp; // pointer to the sampler
		
		bool **plru_bits;    // per-set pseudo-LRU bits
		unsigned char **rrpv;		// for RRIP policy
		int lognsets;

		int psel = 0; // for set-dueling the best bypass threshold
		int config = 1; // the configuration number
		bool was_burst; // most recent access was a cache burst
		int total_bits = 0; // for computig space overhead

    private:
		void set_parameters(uint32_t num_cpus);
        void UpdateSampler (uint32_t setIndex, uint64_t tag, uint32_t tid, uint64_t PC, int32_t way, bool hit, uint32_t accessType, uint64_t paddr);
		int Get_Sampler_Victim ( uint32_t tid, uint32_t setIndex, const BLOCK *current_set, uint32_t assoc, uint64_t PC, uint64_t paddr, uint32_t accessType);
		void update_plru_mdpp (int set, int32_t wayID, bool hit, int *vector, uint32_t accessType, bool really = true, int conf = 0);
		int get_mdpp_plru_victim (int set);
		unsigned int mm (unsigned int x, unsigned int m[]);
        
    public:
        MP3BRepl(string name) : CacheReplBase(name){}
        ~MP3BRepl(){}

        // override fuctions
        void print_config();
        void initialize_replacement();
        void update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit);
        uint32_t find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
        void dump_stats();
};

#endif /* MP3B_H */


