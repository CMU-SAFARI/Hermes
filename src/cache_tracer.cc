#include <iostream>
#include <assert.h>
#include <sstream>
#include "cache_tracer.h"
#include "defs.h"

string access_type_to_str[] = {"LOAD", "RFO", "PREFETCH", "WRITEBACK", "ALL"};

void CacheTracer::init_tracing(string trace_filename, uint32_t type,
                               int32_t cpu)
{
  assert(type <= NUM_TYPES);
  access_type = (uint8_t)type;
  cout << "++++++ Enabling cache access trace dump for access type "
       << access_type_to_str[access_type] << " ++++++" << endl;

  stringstream trace_name;
  if (cpu >= 0) {
    trace_name << trace_filename << ".core" << cpu << ".zst";
  } else {
    trace_name << trace_filename << ".zst";
  }
  trace_file = fopen(trace_name.str().c_str(), "wb");
  assert(trace_file != NULL);

  cctx = ZSTD_createCCtx();
  assert(cctx != NULL);
  out_buf.resize(ZSTD_CStreamOutSize());
  in_buf.reserve(ZSTD_CStreamInSize());
}

// Compresses everything staged. ZSTD_e_end also closes the frame.
void CacheTracer::drain(ZSTD_EndDirective mode)
{
  ZSTD_inBuffer in = {in_buf.data(), in_buf.size(), 0};
  size_t        remaining;

  do {
    ZSTD_outBuffer out = {out_buf.data(), out_buf.size(), 0};
    remaining          = ZSTD_compressStream2(cctx, &out, &in, mode);
    assert(!ZSTD_isError(remaining));
    fwrite(out_buf.data(), 1, out.pos, trace_file);
  } while (mode == ZSTD_e_end ? remaining != 0 : in.pos != in.size);

  in_buf.clear();
}

void CacheTracer::stage(const void *src, size_t len)
{
  const char *p = (const char *)src;
  in_buf.insert(in_buf.end(), p, p + len);
  if (in_buf.size() >= ZSTD_CStreamInSize()) {
    drain(ZSTD_e_continue);
  }
}

void CacheTracer::fini_tracing()
{
  if (trace_file == NULL) {
    return;
  }
  drain(ZSTD_e_end);
  ZSTD_freeCCtx(cctx);
  fclose(trace_file);
  cctx       = NULL;
  trace_file = NULL;
}

// Bypasses the access_type filter: the marker must survive any trace type.
void CacheTracer::record_roi_marker()
{
  if (trace_file == NULL) {
    return;
  }
  uint64_t address = TRACE_ROI_MARKER_ADDR;
  uint8_t  type    = TRACE_ROI_MARKER_TYPE;
  bool     hit     = false;
  stage(&address, sizeof(uint64_t));
  stage(&type, sizeof(uint8_t));
  stage(&hit, sizeof(bool));
}

void CacheTracer::record_trace(uint64_t address, uint8_t type, bool hit)
{
  if (access_type == NUM_TYPES || type == access_type) {
    stage(&address, sizeof(uint64_t));
    stage(&type, sizeof(uint8_t));
    stage(&hit, sizeof(bool));
  }
}
