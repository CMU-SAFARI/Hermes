#ifndef CACHE_TRACER_H
#define CACHE_TRACER_H

#include <stdint.h>
#include <string>
#include "zlib.h"

using namespace std;

class CacheTracer
{
    private:
        uint8_t access_type = 0;
        gzFile trace_file = Z_NULL;

    public:
        void init_tracing(string filename, uint32_t type, int32_t cpu = -1);
        void fini_tracing();
        void record_trace(uint64_t address, uint8_t type, bool hit);
};

#endif /* CACHE_TRACER_H */

