#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

void gen_random(char *s, const int len);
uint32_t folded_xor(uint64_t value, uint32_t num_folds);

template <class T> std::string array_to_string(std::vector<T> array, bool hex = false, uint32_t size = 0)
{
    std::stringstream ss;
    if (size == 0) size = array.size();
    for(uint32_t index = 0; index < size; ++index)
    {
    	if(hex)
    	{
    		ss << std::hex << array[index] << std::dec;
    	}
    	else
    	{
    		ss << array[index];
    	}
        ss << ",";
    }
    return ss.str();
}

class HashZoo
{
public:
	static uint32_t jenkins(uint32_t key);
	static uint32_t knuth(uint32_t key);
	static uint32_t murmur3(uint32_t key);
	static uint32_t jenkins32(uint32_t key);
	static uint32_t hash32shift(uint32_t key);
	static uint32_t hash32shiftmult(uint32_t key);
	static uint32_t hash64shift(uint32_t key);
	static uint32_t hash5shift(uint32_t key);
	static uint32_t hash7shift(uint32_t key);
	static uint32_t Wang6shift(uint32_t key);
	static uint32_t Wang5shift(uint32_t key);
	static uint32_t Wang4shift(uint32_t key);
	static uint32_t Wang3shift(uint32_t key);
    static uint32_t xxhash32(uint32_t key);
    static uint32_t fnv1a32(uint32_t key);

    static uint32_t three_hybrid1(uint32_t key);
    static uint32_t three_hybrid2(uint32_t key);
    static uint32_t three_hybrid3(uint32_t key);
    static uint32_t three_hybrid4(uint32_t key);
    static uint32_t three_hybrid5(uint32_t key);
    static uint32_t three_hybrid6(uint32_t key);
    static uint32_t three_hybrid7(uint32_t key);
    static uint32_t three_hybrid8(uint32_t key);
    static uint32_t three_hybrid9(uint32_t key);
    static uint32_t three_hybrid10(uint32_t key);
    static uint32_t three_hybrid11(uint32_t key);
    static uint32_t three_hybrid12(uint32_t key);

    static uint32_t four_hybrid1(uint32_t key);
    static uint32_t four_hybrid2(uint32_t key);
    static uint32_t four_hybrid3(uint32_t key);
    static uint32_t four_hybrid4(uint32_t key);
    static uint32_t four_hybrid5(uint32_t key);
    static uint32_t four_hybrid6(uint32_t key);
    static uint32_t four_hybrid7(uint32_t key);
    static uint32_t four_hybrid8(uint32_t key);
    static uint32_t four_hybrid9(uint32_t key);
    static uint32_t four_hybrid10(uint32_t key);
    static uint32_t four_hybrid11(uint32_t key);
    static uint32_t four_hybrid12(uint32_t key);

    /* some 64-bit hash functions */
    static uint64_t crc64(uint64_t key);
    static uint64_t xxhash64(uint64_t key);
    static uint64_t fnv1a64(uint64_t key);

    static uint32_t getHash(uint32_t selector, uint32_t key);
};

/* defines a saturated counter */
class Counter
{
    private:
        uint32_t width = 0;
        uint64_t value = 0;
        uint64_t max_value = 0;

    public:
        Counter() : width(0), value(0), max_value(0) {}
        Counter(uint32_t w, uint64_t init_val = 0)
        {
            init(w, init_val);
        }
        ~Counter(){}
        inline void init(uint32_t w, uint64_t init_val = 0){
            width = w;
            max_value = (1ul << width) - 1;
            value = init_val;
        }
        inline void incr(uint64_t amt = 1){
            value += amt;
            if(value >= max_value) value = max_value;
        }
        inline void decr(uint64_t amt = 1){
            if(value >= amt) value -= amt; else value = 0;
        }
        inline uint64_t val() {return value;}
        inline void     reset() {value = 0;}
        inline uint64_t max() {return max_value;}
};

#endif /* UTIL_H */

