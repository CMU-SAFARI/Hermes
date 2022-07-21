#include <iostream>
#include <assert.h>
#include <sstream>
#include "cache_tracer.h"
#include "defs.h"

string access_type_to_str[] = {"LOAD", "RFO", "PREFETCH", "WRITEBACK", "ALL"};

void CacheTracer::init_tracing(string trace_filename, uint32_t type, int32_t cpu)
{
    assert(type <= NUM_TYPES);
    access_type = (uint8_t)type;
    cout << "++++++ Enabling cache access trace dump for access type " << access_type_to_str[access_type] << " ++++++" << endl;
    
    stringstream trace_name;
    if(cpu >= 0) 
    {
        trace_name << trace_filename << ".core" << cpu << ".gz";
    }
    else
    {
        trace_name << trace_filename << ".gz";
    }
    trace_file = gzopen(trace_name.str().c_str(), "wb");
    assert(trace_file != Z_NULL);
}

void CacheTracer::fini_tracing()
{
    if(trace_file != Z_NULL)
    {
        gzflush(trace_file, Z_FINISH);
        gzclose(trace_file);
    }
}

void CacheTracer::record_trace(uint64_t address, uint8_t type, bool hit)
{
    if(access_type == NUM_TYPES || type == access_type)
    {
        // cout << "Addr: " << hex << address << dec << " type " << (uint32_t)type << " hit " << hit << endl;
        gzwrite(trace_file, (void*)(&address), sizeof(uint64_t));
        gzwrite(trace_file, (void*)(&type), sizeof(uint8_t));
        gzwrite(trace_file, (void*)(&hit), sizeof(bool));
    }
}