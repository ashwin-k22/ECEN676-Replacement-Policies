#include "seg_lru.h"

#include <algorithm>
#include <cassert>

seg_lru::seg_lru(CACHE* cache) 
    : seg_lru(cache, cache->NUM_SET, cache->NUM_WAY) {}

seg_lru::seg_lru(CACHE* cache, long sets, long ways) 
    : replacement(cache), 
      NUM_WAY(ways), 
      last_used_cycles(static_cast<std::size_t>(sets * ways), 0) {}

long seg_lru::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, 
                      const champsim::cache_block* current_set, 
                      champsim::address ip, champsim::address full_addr, 
                      access_type type)
{
    auto begin = std::next(std::begin(last_used_cycles), set * NUM_WAY);
    auto end = std::next(begin, NUM_WAY);

    // Find the victim by selecting the least recently used (inactive first)
    auto victim = std::min_element(begin, end);
    return std::distance(begin, victim);
}

void seg_lru::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, 
                                 champsim::address full_addr, champsim::address ip, 
                                 champsim::address victim_addr, access_type type)
{
    // Insert as an inactive page (low cycle value)
    last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = cycle++; 
}

void seg_lru::update_replacement_state(uint32_t triggering_cpu, long set, long way, 
                                   champsim::address full_addr, champsim::address ip, 
                                   champsim::address victim_addr, access_type type, 
                                   uint8_t hit)
{
    auto idx = (std::size_t)(set * NUM_WAY + way);

    if (hit && access_type{type} != access_type::WRITE) { // Skip for writeback hits
        // If the entry was inactive (low cycle value), promote it by boosting its cycle count
        if (last_used_cycles.at(idx) < cycle - NUM_WAY) {
            last_used_cycles.at(idx) = cycle + NUM_WAY; // Boost cycle count for active promotion
        } else {
            last_used_cycles.at(idx) = cycle++; // Regular update for active pages
        }
    }
}
