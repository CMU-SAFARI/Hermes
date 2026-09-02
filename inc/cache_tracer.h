#ifndef CACHE_TRACER_H
#define CACHE_TRACER_H

#include <stdint.h>
#include <string>
#include "zlib.h"

using namespace std;

// ROI boundary marker written into the access trace. Type 255 is outside
// NUM_TYPES, so it cannot collide with a real record. tools/optcache reads it.
#define TRACE_ROI_MARKER_TYPE 255
#define TRACE_ROI_MARKER_ADDR 0xdeadbeefULL

class CacheTracer
{
private:
  uint8_t access_type = 0;
  gzFile  trace_file  = Z_NULL;

public:
  void init_tracing(string filename, uint32_t type, int32_t cpu = -1);
  void fini_tracing();
  void record_trace(uint64_t address, uint8_t type, bool hit);
  void record_roi_marker();
};

#endif /* CACHE_TRACER_H */
