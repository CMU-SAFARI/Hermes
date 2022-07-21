#include <iostream>
#include <assert.h>
#include "offchip_tracer.h"
using namespace std;

void OffchipTracer::init_tracing(string _filename, uint32_t cpu)
{
    filename = _filename;
    cout << "+++++++ Enabling offchip latency tracing in CPU " << cpu << " +++++++" << endl;
}

void OffchipTracer::fini_tracing()
{
    trace_file = fopen(filename.c_str(), "w");
    assert(trace_file);

    cerr << "Dumping offchip latency trace in " << filename << ". May take a while..." << endl;
    dump_trace();
    
    if(trace_file)
    {
        fflush(trace_file);
        fclose(trace_file);
    }
}

void OffchipTracer::record(uint64_t latency)
{
    auto it = latency_map.find(latency);
    if(it != latency_map.end())
    {
        it->second++;
    }
    else
    {
        latency_map.insert(pair<uint64_t,uint64_t>(latency, 1));
    }
    // fprintf(trace_file, "%ld\n", latency);
}

void OffchipTracer::dump_trace()
{
    assert(trace_file);
    fprintf(trace_file, "%s\n", filename.c_str());
    for(auto it = latency_map.begin(); it != latency_map.end(); ++it)
    {
        uint64_t val = it->first;
        uint64_t count = it->second;
        for(uint32_t i = 0; i < count; ++i)
        {
            fprintf(trace_file, "%ld\n", val);
        }
    }
}