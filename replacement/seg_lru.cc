#include "seg_lru.h"

#include <algorithm>
#include <cassert>
#include <vector>

class seg_lru : public replacement
{
private:
    long NUM_WAY;
    std::vector<uint64_t> last_used_cycles;  // Track last used cycle for LRU
    std::vector<bool> outcome_bits;          // Track whether a line has been re-referenced

public:
    seg_lru(CACHE* cache) : seg_lru(cache, cache->NUM_SET, cache->NUM_WAY) {}

    seg_lru(CACHE* cache, long sets, long ways) 
        : replacement(cache), NUM_WAY(ways),
          last_used_cycles(static_cast<std::size_t>(sets * ways), 0),
          outcome_bits(static_cast<std::size_t>(sets * ways), false) {}

    long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                      champsim::address full_addr, access_type type) override
    {
        auto begin = std::next(std::begin(last_used_cycles), set * NUM_WAY);
        auto end = std::next(begin, NUM_WAY);

        // First, check for a cache line with outcome bit == false
        for (long way = 0; way < NUM_WAY; ++way)
        {
            if (!outcome_bits[set * NUM_WAY + way])
                return way;  // Select this as the victim
        }

        // If no such line exists, fall back to traditional LRU selection
        auto victim = std::min_element(begin, end);
        return std::distance(begin, victim);
    }

    void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                 access_type type) override
    {
        // On insertion, reset the outcome bit and update last used cycle
        long index = set * NUM_WAY + way;
        outcome_bits[index] = false;  // New line, so outcome is false
        last_used_cycles[index] = cycle++;
    }

    void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type, uint8_t hit) override
    {
        long index = set * NUM_WAY + way;
        
        if (hit && access_type{type} != access_type::WRITE) // Skip for writeback hits
        {
            outcome_bits[index] = true;  // Mark it as re-referenced
            last_used_cycles[index] = cycle++;
        }
    }
};
