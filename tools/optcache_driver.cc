#ifndef OPTCACHE_DRIVER_H
#define OPTCACHE_DRIVER_H

#include <zlib.h>
#include "optcache.h"

#define LOG_INTERVAL 1000000

int main(int argc, char **argv)
{
    cout << "**********************************************************" << endl
         << "                 Belady' OPT Simulation                   " << endl
         << "          Last compiled: " << __DATE__ << " " << __TIME__ << endl
         << "**********************************************************" << endl;
    
    if(argc < 6)
    {
        cout << "Usage: ./optcache_driver <sets> <assoc> <cache_trace_file> <reuse_dist_file> <bypass>" << endl;
        assert(false);
    }

    uint32_t sets = atoi(argv[1]);
    uint32_t assoc = atoi(argv[2]);
    string cache_trace_filename = string(argv[3]);
    string reuse_dist_filename = string(argv[4]);
    bool bypass_en = atoi(argv[5]) != 0 ? true : false;

    cout << "Sets " << sets << endl
        << "Assoc " << assoc << endl
        << "Size " << (sets*assoc*64)/1024 << endl
        << "Bypass_enabled " << bypass_en << endl
        << "Cache_trace_file " << cache_trace_filename  << endl
        << "Reuse_dist_file " << reuse_dist_filename << endl
        << endl;

    // init optcache
    OptCache *optcache = new OptCache(sets, assoc, bypass_en);
    assert(optcache);

    gzFile cache_trace_file = gzopen(cache_trace_filename.c_str(), "rb");
    assert(cache_trace_file);
    gzFile reuse_dist_file = gzopen(reuse_dist_filename.c_str(), "rb");
    assert(reuse_dist_file);

    uint64_t address, reuse_dist;
    uint8_t type;
    bool hit;

    uint64_t counter = 0;

    while(!gzeof(cache_trace_file))
    {
        gzread(cache_trace_file, (void*)(&address), sizeof(uint64_t));
        gzread(cache_trace_file, (void*)(&type), sizeof(uint8_t));
        gzread(cache_trace_file, (void*)(&hit), sizeof(bool));
        gzread(reuse_dist_file, (void*)(&reuse_dist), sizeof(uint64_t));

        optcache->access(address, type, hit, reuse_dist);

        counter++;
        if(counter % LOG_INTERVAL == 0)
        {
            cout << "Processed " << counter << endl;
        }
    }

    gzclose(cache_trace_file);
    gzclose(reuse_dist_file);

    cout << endl;
    optcache->dump_stats();

    return 0;
}

#endif /* OPTCACHE_DRIVER_H */

