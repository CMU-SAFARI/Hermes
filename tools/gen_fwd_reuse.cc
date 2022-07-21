#ifndef GEN_FWD_REUSE_H
#define GEN_FWD_REUSE_H

#include <iostream>
#include <string>
#include <zlib.h>
#include <assert.h>
#include <vector>
#include <unordered_map>

using namespace std;

#define LOG2_BLOCK_SIZE 6
#define LOG_INTERVAL 1000000

int main(int argc, char** argv)
{
    cout << "**********************************************************" << endl
         << "                 Forward Reuse Generator                  " << endl
         << "          Last compiled: " << __DATE__ << " " << __TIME__ << endl
         << "**********************************************************" << endl;
    
    if(argc < 3)
    {
        cout << "Usage: ./gen_fwd_reuse.sh <infile> <outfile>";
        assert(false);
    }

    string cache_access_trace_filename = string(argv[1]);
    string out_filename = string(argv[2]) + ".reuse.gz";

    gzFile infile = gzopen(cache_access_trace_filename.c_str(), "rb");
    assert(infile);

    uint64_t address;
    uint8_t type;
    bool hit;

    uint64_t timestamp = 0;
    vector<uint64_t> reuse_dist;
    unordered_map<uint64_t, uint64_t> age_map;

    cout << "Started reading trace " << cache_access_trace_filename << endl;

    while(!gzeof(infile))
    {
        gzread(infile, (void*)(&address), sizeof(uint64_t));
        gzread(infile, (void*)(&type), sizeof(uint8_t));
        gzread(infile, (void*)(&hit), sizeof(bool));

        // cout << "Addr: " << hex << address << dec << " type " << (uint32_t)type << " hit " << hit << endl;
        
        uint64_t ca_address = (address >> LOG2_BLOCK_SIZE) << LOG2_BLOCK_SIZE;

        // look for the previous occurence of the same address
        auto it = age_map.find(ca_address);
        if(it != age_map.end()) // this means the address has occurred before
        {
            uint64_t reuse = (timestamp - it->second);
            
            // update the reuse distance of the previous occurence
            assert(it->second < reuse_dist.size());
            reuse_dist[it->second] = reuse;

            // now udpate the timestamp in the age map to point the recent occurence
            it->second = timestamp;
        }
        else // this is the first occurence of the address
        {
            age_map.insert(pair<uint64_t, uint64_t>(ca_address, timestamp));
        }

        // add an entry in the reuse distance array for the current occurence
        reuse_dist.push_back(0);

        // increment the timestamp
        timestamp++;

        // logging
        if(timestamp % LOG_INTERVAL == 0)
        {
            cout << "processed " << timestamp << endl;
        }
    }
    gzclose(infile);

    assert(reuse_dist.size() == timestamp);

    cout << "Reuse generation complete. Dumping reuse distances..." << endl;

    // dump the reuse_distance array
    gzFile outfile = gzopen(out_filename.c_str(), "wb");
    assert(outfile);
    for(uint32_t index = 0; index < reuse_dist.size(); ++index)
    {
        uint64_t reuse = reuse_dist[index] > 0 ? reuse_dist[index] : UINT64_MAX;
        gzwrite(outfile, (void*)(&reuse), sizeof(uint64_t));
        // cout << "Reuse dist " << reuse << endl;
    }
    gzflush(outfile, Z_FINISH);
    gzclose(outfile);

    cout << "Reuse generation finished" << endl
        << "Total_processed " << timestamp << endl
        << endl;

    return 0;
}

#endif /* GEN_FWD_REUSE_H */

