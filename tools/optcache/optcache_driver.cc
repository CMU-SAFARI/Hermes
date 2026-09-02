#ifndef OPTCACHE_DRIVER_H
#define OPTCACHE_DRIVER_H

#include "optcache.h"
#include "zstd_file.h"

#define LOG_INTERVAL 1000000

int main(int argc, char **argv)
{
  cout << "**********************************************************" << endl
       << "                 Belady' OPT Simulation                   " << endl
       << "          Last compiled: " << __DATE__ << " " << __TIME__ << endl
       << "**********************************************************" << endl;

  if (argc < 6) {
    cout << "Usage: ./optcache_driver <sets> <assoc> <cache_trace_file> "
            "<reuse_dist_file> <bypass> [--roi-marker]"
         << endl;
    assert(false);
  }

  uint32_t sets                 = atoi(argv[1]);
  uint32_t assoc                = atoi(argv[2]);
  string   cache_trace_filename = string(argv[3]);
  string   reuse_dist_filename  = string(argv[4]);
  bool     bypass_en            = atoi(argv[5]) != 0 ? true : false;

  bool roi_marker_en = false;
  for (int i = 6; i < argc; ++i) {
    if (string(argv[i]) == "--roi-marker") {
      roi_marker_en = true;
    }
  }

  cout << "Sets " << sets << endl
       << "Assoc " << assoc << endl
       << "Size " << (sets * assoc * 64) / 1024 << endl
       << "Bypass_enabled " << bypass_en << endl
       << "Roi_marker_enabled " << roi_marker_en << endl
       << "Cache_trace_file " << cache_trace_filename << endl
       << "Reuse_dist_file " << reuse_dist_filename << endl
       << endl;

  // init optcache
  OptCache *optcache = new OptCache(sets, assoc, bypass_en);
  assert(optcache);

  ZstdReader cache_trace_file, reuse_dist_file;
  cache_trace_file.open(cache_trace_filename);
  reuse_dist_file.open(reuse_dist_filename);

  uint64_t address, reuse_dist;
  uint8_t  type;
  bool     hit;

  uint64_t counter = 0;

  while (cache_trace_file.read(&address, sizeof(uint64_t)) &&
         cache_trace_file.read(&type, sizeof(uint8_t)) &&
         cache_trace_file.read(&hit, sizeof(bool))) {
    if (!reuse_dist_file.read(&reuse_dist, sizeof(uint64_t))) {
      cout << "ERROR: reuse file ends at record " << counter
           << " but the trace continues; regenerate it from this trace" << endl;
      assert(false);
    }

    if (type == ROI_MARKER_TYPE) {
      assert(address == ROI_MARKER_ADDR);
      // Never pass a marker to access(): it indexes stats[] by type.
      if (!roi_marker_en) {
        cout << "ERROR: ROI marker at record " << counter
             << " but --roi-marker not given; stats would include warmup"
             << endl;
        assert(false);
      }
      optcache->reset_stats();
      cout << "ROI marker at record " << counter
           << ": stats reset, cache left warm" << endl;
      counter++;
      continue;
    }

    optcache->access(address, type, hit, reuse_dist, counter);

    counter++;
    if (counter % LOG_INTERVAL == 0) {
      cout << "Processed " << counter << endl;
    }
  }

  cache_trace_file.close();
  reuse_dist_file.close();

  cout << endl;
  optcache->dump_stats();

  return 0;
}

#endif /* OPTCACHE_DRIVER_H */
