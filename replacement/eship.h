#ifndef REPLACEMENT_ESHIP_H
#define REPLACEMENT_ESHIP_H

#include <array>
#include <vector>
#include <unordered_map>
#include <deque>

#include "cache.h"
#include "modules.h"
#include "msl/bits.h"
#include "msl/fwcounter.h"

// ESHIP replacement policy
struct eship : public champsim::modules::replacement {
private:
  int& get_rrpv(long set, long way);
  uint32_t get_signature(champsim::address ip);
  void update_stats(uint32_t cpu, bool hit);
  void adjust_prediction_weights(uint32_t cpu);

public:
  // core ESHIP parameters
  static constexpr int maxRRPV = 7;
  static constexpr std::size_t SHCT_SIZE = 16384;
  static constexpr unsigned SHCT_PRIME = 16381;
  static constexpr std::size_t SAMPLER_SET_FACTOR = 256;
  static constexpr unsigned SHCT_MAX = 15;

  // enhanced parameters
  static constexpr unsigned HISTORY_SIZE = 128;
  static constexpr unsigned PATTERN_SIZE = 16;
  static constexpr unsigned FREQUENCY_MAX = 31;

  // access history tracker
  struct AccessPattern {
    bool hit;
    champsim::address addr;
    champsim::address ip;
    access_type type;
  };

  // enhanced sampler structure
  class SAMPLER_class {
  public:
    bool valid = false;
    bool used = false;
    champsim::address address{};
    champsim::address ip{};
    uint64_t last_used = 0;
    uint64_t frequency = 0;  // track access frequency
    access_type last_type{}; // track last access type
  };

  long NUM_SET, NUM_WAY;
  uint64_t access_count = 0;

  // performance tracking
  std::vector<uint64_t> hits;
  std::vector<uint64_t> accesses;
  std::vector<double> hit_ratio;
  std::vector<std::deque<AccessPattern>> access_history;

  // sampler
  std::vector<std::size_t> rand_sets;
  std::vector<SAMPLER_class> sampler;
  std::vector<int> rrpv_values;
  std::vector<int> frequency_counters; // track frequency for each block

  // enhanced prediction tables
  std::vector<std::array<champsim::msl::fwcounter<champsim::msl::lg2(SHCT_MAX + 1)>, SHCT_SIZE>> SHCT;
  std::vector<std::array<champsim::msl::fwcounter<champsim::msl::lg2(FREQUENCY_MAX + 1)>, SHCT_SIZE>> frequency_table;
  std::vector<std::array<float, 4>> type_weights; // weights for different access types

  explicit eship(CACHE* cache);

  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set,
                  champsim::address ip, champsim::address full_addr, access_type type);

  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                              champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit);

  void replacement_final_stats();
};

#endif
