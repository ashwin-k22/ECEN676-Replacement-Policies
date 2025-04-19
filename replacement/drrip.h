// drrip.h
#ifndef REPLACEMENT_DRRIP_H
#define REPLACEMENT_DRRIP_H

#include <vector>
#include "cache.h"
#include "modules.h"
#include "msl/fwcounter.h"

// DRRIP (Dynamic Re-Reference Interval Prediction) replacement policy class
struct drrip : public champsim::modules::replacement {
private:
  unsigned& get_rrpv(long set, long way);

public:
  static constexpr unsigned MAX_RRPV = 3; // Maximum RRPV value
  static constexpr std::size_t BIP_MAX = 32; // Probability denominator for BIP (Bimodal Insertion Policy)
  static constexpr std::size_t SDM_SIZE = 32; // Size of Set Dueling Monitor (SDM) groups
  static constexpr std::size_t PSEL_WIDTH = 10; // Width of the PSEL (Policy Selector) counter
  static constexpr std::size_t HIT_RATE_SAMPLE_INTERVAL = 100000; // Interval for sampling hit rate

  
  unsigned NUM_SET = 0; 
  unsigned NUM_WAY = 0;

  std::vector<unsigned> rrpv; // RRPV table for all cache blocks
  std::vector<std::size_t> sdm_sets; // List of SDM sets
  std::vector<champsim::msl::fwcounter<PSEL_WIDTH>> PSEL; // PSEL counters for SDM groups

  
  uint64_t access_counter = 0; // Total number of cache accesses
  uint64_t hit_counter = 0; // Total number of cache hits
  uint64_t miss_counter = 0; // Total number of cache misses

  uint64_t bip_inserts = 0; // Number of BIP insertions
  uint64_t srrip_inserts = 0; // Number of SRRIP insertions
  uint64_t fill_counter = 0; // Number of cache fills
  uint64_t reuse_on_victim = 0; // Number of times a victim block was reused
  uint64_t rrpv_sum_on_hits = 0; // Sum of RRPV values on cache hits
  uint64_t rrpv_hit_count = 0; // Number of cache hits used for RRPV statistics

  std::vector<double> hit_rate_history; // History of sampled hit rates

  // Constructor to initialize the DRRIP replacement policy
  drrip(CACHE* cache);

  // Function to find a victim cache block for replacement
  long find_victim(uint32_t cpu, uint64_t instr_id, long set, const champsim::cache_block* set_blocks,
                   champsim::address ip, champsim::address addr, access_type type);

  // Function to update the replacement state after a cache access
  void update_replacement_state(uint32_t cpu, long set, long way, champsim::address addr, champsim::address ip,
                                champsim::address victim_addr, access_type type, uint8_t hit);

  // Function to update the state for BIP (Bimodal Insertion Policy)
  void update_bip(long set, long way);

  // Function to update the state for SRRIP (Static Re-Reference Interval Prediction)
  void update_srrip(long set, long way);
};

#endif
