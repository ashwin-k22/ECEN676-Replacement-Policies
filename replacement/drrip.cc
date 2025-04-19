#include "drrip.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <algorithm>

// Constructor for the drrip class, initializes cache parameters and data structures
drrip::drrip(CACHE* cache)
  : champsim::modules::replacement(cache)
{
  NUM_SET = 2048; // Number of sets in the cache
  NUM_WAY = 16;   // Number of ways in the cache

  // Initialize RRPV (Re-Reference Prediction Value) table with maximum values
  rrpv.resize(NUM_SET * NUM_WAY, MAX_RRPV);

  // Initialize PSEL (Policy Selector) counters for each SDM group
  PSEL.resize(NUM_SET / SDM_SIZE);

  // Identify SDM (Set Dueling Monitor) sets
  for (std::size_t i = 0; i < NUM_SET; i += SDM_SIZE)
    sdm_sets.push_back(i);
}

// Helper function to get the RRPV value for a specific set and way
unsigned& drrip::get_rrpv(long set, long way) {
  return rrpv[set * NUM_WAY + way];
}

// Function to find a victim cache block for replacement
long drrip::find_victim(uint32_t cpu, uint64_t instr_id, long set,
                        const champsim::cache_block* set_blocks,
                        champsim::address ip, champsim::address addr,
                        access_type type)
{
  while (true) {
    // Search for a block with the maximum RRPV value (victim candidate)
    for (long way = 0; way < NUM_WAY; ++way) {
      if (get_rrpv(set, way) == MAX_RRPV)
        return way;
    }

    // Increment RRPV values for all blocks if no victim is found
    for (long way = 0; way < NUM_WAY; ++way) {
      if (get_rrpv(set, way) < MAX_RRPV)
        get_rrpv(set, way)++;
    }
  }
}

// Function to update the replacement state after a cache access
void drrip::update_replacement_state(uint32_t cpu, long set, long way,
                                     champsim::address addr, champsim::address ip,
                                     champsim::address victim_addr, access_type type,
                                     uint8_t hit)
{
  access_counter++; // Increment the total access counter

  if (hit) {
    // Update state for a cache hit
    hit_counter++;
    rrpv_sum_on_hits += get_rrpv(set, way);
    rrpv_hit_count++;
    get_rrpv(set, way) = 0; // Reset RRPV for the accessed block
  } else {
    // Update state for a cache miss
    miss_counter++;
    fill_counter++;
    if (get_rrpv(set, way) == 0)
      reuse_on_victim++;

    // Determine whether to use SRRIP or BIP policy
    bool use_srrip = false;
    std::size_t sdm_group = set / SDM_SIZE;

    if (std::find(sdm_sets.begin(), sdm_sets.end(), set) != sdm_sets.end()) {
      // SDM sets alternate between SRRIP and BIP
      if (sdm_group % 2 == 0) {
        use_srrip = true;
        ++PSEL[sdm_group]; // Favor SRRIP
      } else {
        use_srrip = false;
        --PSEL[sdm_group]; // Favor BIP
      }
    } else {
      // Use the policy determined by the PSEL counter
      use_srrip = PSEL[sdm_group].value() > (1 << (PSEL_WIDTH - 1));
    }

    // Update the replacement state using the selected policy
    if (use_srrip) update_srrip(set, way);
    else update_bip(set, way);
  }

  // Periodically record the hit rate
  if (access_counter % HIT_RATE_SAMPLE_INTERVAL == 0) {
    double rate = 100.0 * static_cast<double>(hit_counter) / static_cast<double>(access_counter);
    hit_rate_history.push_back(rate);
  }
}

// Function to update the state for SRRIP (Static Re-Reference Interval Prediction)
void drrip::update_srrip(long set, long way) {
  get_rrpv(set, way) = MAX_RRPV - 1; // Set RRPV to a high value (but not max)
  srrip_inserts++; // Increment SRRIP insertion counter
}

// Function to update the state for BIP (Bimodal Insertion Policy)
void drrip::update_bip(long set, long way) {
  static std::mt19937 gen(std::random_device{}()); // Random number generator
  static std::uniform_int_distribution<int> dist(0, BIP_MAX - 1); // Distribution for BIP decision

  // Insert with a low probability (bimodal behavior)
  if (dist(gen) == 0)
    get_rrpv(set, way) = MAX_RRPV - 1; // Favor SRRIP-like behavior
  else
    get_rrpv(set, way) = MAX_RRPV; // Favor BIP-like behavior

  bip_inserts++; // Increment BIP insertion counter
}
