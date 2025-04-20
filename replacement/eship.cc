#include "eship.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <cmath>
#include <iostream>

#include "champsim.h"

// initialize replacement state
eship::eship(CACHE* cache)
    : replacement(cache),
      NUM_SET(cache->NUM_SET),
      NUM_WAY(cache->NUM_WAY),
      hits(NUM_CPUS, 0),
      accesses(NUM_CPUS, 0),
      hit_ratio(NUM_CPUS, 0.0),
      access_history(NUM_CPUS),
      sampler(SAMPLER_SET_FACTOR * NUM_CPUS * static_cast<std::size_t>(NUM_WAY)),
      rrpv_values(static_cast<std::size_t>(NUM_SET * NUM_WAY), maxRRPV),
      frequency_counters(static_cast<std::size_t>(NUM_SET * NUM_WAY), 0)
{
    // randomly selected sampler sets
    std::knuth_b rng(1);
    std::generate_n(std::back_inserter(rand_sets), SAMPLER_SET_FACTOR * NUM_CPUS, [&rng, this]() { return rng() % NUM_SET; });
    std::sort(std::begin(rand_sets), std::end(rand_sets));

    // initialize prediction tables
    std::generate_n(std::back_inserter(SHCT), NUM_CPUS, []() -> typename decltype(SHCT)::value_type { return {}; });
    std::generate_n(std::back_inserter(frequency_table), NUM_CPUS, []() -> typename decltype(frequency_table)::value_type { return {}; });

    // initialize access type weights starting with balanced weights
    for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
        type_weights.push_back({
            1.0f, // LOAD
            0.8f, // RFO
            0.5f, // PREFETCH
            0.3f  // WRITE
        });

        // initialize access history deques with appropriate size
        access_history[cpu].resize(HISTORY_SIZE);
    }
}

int& eship::get_rrpv(long set, long way) {
    return rrpv_values.at(static_cast<std::size_t>(set * NUM_WAY + way));
}

uint32_t eship::get_signature(champsim::address ip) {
    // create signature from the IP
    // use folding hash to create distribution
    using namespace champsim::data::data_literals;
    uint32_t sig = ip.slice_lower<champsim::data::bits{32}>().to<uint32_t>();
    sig = ((sig >> 16) ^ sig) * 0x45d9f3b;
    sig = ((sig >> 16) ^ sig) * 0x45d9f3b;
    sig = ((sig >> 16) ^ sig);
    return sig % SHCT_PRIME;
}

void eship::update_stats(uint32_t cpu, bool hit) {
    // update hit/access statistics
    accesses[cpu]++;
    if (hit) hits[cpu]++;

    // recalculate hit ratio every 1000 accesses
    if (accesses[cpu] % 1000 == 0) {
        hit_ratio[cpu] = static_cast<double>(hits[cpu]) / accesses[cpu];

        // adjust prediction weights after collecting data
        if (accesses[cpu] >= 10000) {
            adjust_prediction_weights(cpu);
        }
    }
}

void eship::adjust_prediction_weights(uint32_t cpu) {
    // count frequency of each access type in history
    uint32_t type_counts[4] = {0, 0, 0, 0};
    uint32_t type_hits[4] = {0, 0, 0, 0};

    for (const auto& access : access_history[cpu]) {
        uint32_t type_idx = static_cast<uint32_t>(access.type);
        if (type_idx < 4) {
            type_counts[type_idx]++;
            if (access.hit) type_hits[type_idx]++;
        }
    }

    // adjust weights based on hit rate of each type
    for (uint32_t i = 0; i < 4; i++) {
        if (type_counts[i] > 0) {
            float hit_rate = static_cast<float>(type_hits[i]) / type_counts[i];
            // higher hit rate means more valuable access type
            type_weights[cpu][i] = 0.5f + hit_rate / 2.0f;
        }
    }
}

// find replacement victim
long eship::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set,
                       champsim::address ip, champsim::address full_addr, access_type type)
{
    // look for maxRRPV line
    auto begin = std::next(std::begin(rrpv_values), set * NUM_WAY);
    auto end = std::next(begin, NUM_WAY);
    auto freq_begin = std::next(std::begin(frequency_counters), set * NUM_WAY);

    // victim selection:
    // look for invalid lines first (RRPV = maxRRPV)
    auto victim = std::find(begin, end, maxRRPV);

    if (victim != end) {
        return std::distance(begin, victim);
    }

    // prioritize high RRPV and low frequency
    std::vector<std::pair<long, float>> candidates;
    for (long way = 0; way < NUM_WAY; way++) {
        int rrpv = *(begin + way);
        int freq = *(freq_begin + way);

        // higher score is more likely to be evicted
        float score = rrpv * 2.0f - std::log2(freq + 1) / 2.0f;
        candidates.push_back({way, score});
    }

    // find highest score (suitable for eviction)
    auto best_victim = std::max_element(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    return best_victim->first;
}

// called on every cache hit and cache fill
void eship::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                   champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit)
{
    using namespace champsim::data::data_literals;

    // update statistics
    update_stats(triggering_cpu, hit);

    // add to access history
    AccessPattern current_access = {static_cast<bool>(hit), full_addr, ip, type};
    access_history[triggering_cpu].push_back(current_access);
    if (access_history[triggering_cpu].size() > HISTORY_SIZE) {
        access_history[triggering_cpu].pop_front();
    }

    // get index into frequency counter array
    auto freq_idx = static_cast<std::size_t>(set * NUM_WAY + way);

    // handle writeback access
    if (access_type{type} == access_type::WRITE) {
        // writebacks given medium priority
        if (!hit) {
            get_rrpv(set, way) = maxRRPV / 2;
            frequency_counters[freq_idx] = 1;  // init frequency
        } else {
            get_rrpv(set, way) = std::max(0, get_rrpv(set, way) - 1);
            frequency_counters[freq_idx] = std::min(static_cast<int>(FREQUENCY_MAX),
                                                   static_cast<int>(frequency_counters[freq_idx] + 1));
        }
        return;
    }

    // update sampler
    auto s_idx = std::find(std::begin(rand_sets), std::end(rand_sets), set);
    if (s_idx != std::end(rand_sets)) {
        auto s_set_begin = std::next(std::begin(sampler), std::distance(std::begin(rand_sets), s_idx));
        auto s_set_end = std::next(s_set_begin, NUM_WAY);

        // check hit
        auto match = std::find_if(s_set_begin, s_set_end, [addr = full_addr, shamt = champsim::data::bits{8 + champsim::lg2(NUM_WAY)}](auto x) {
            return x.valid && x.address.slice_upper(shamt) == addr.slice_upper(shamt);
        });

        // if hit in sampler
        if (match != s_set_end) {
            auto SHCT_idx = get_signature(match->ip);

            // decrement SHCT based on hit success prediction
            if (match->used) {
                // correct prediction - decrement SHCT
                SHCT[triggering_cpu][SHCT_idx]--;
            } else {
                // incorrect prediction - increment SHCT
                SHCT[triggering_cpu][SHCT_idx]++;
            }

            // update frequency tracking
            frequency_table[triggering_cpu][SHCT_idx]++;

            match->used = true;
            match->frequency++;
            match->last_type = type;

        } else {
            // if miss in sampler
            match = std::min_element(s_set_begin, s_set_end, [](auto x, auto y) {
                // consider both LRU and frequency
                if (!x.valid) return true;
                if (!y.valid) return false;
                return (x.last_used / 2 - x.frequency) < (y.last_used / 2 - y.frequency);
            });

            // if replacing a valid entry
            if (match->valid && match->used) {
                auto SHCT_idx = get_signature(match->ip);
                // Increment SHCT - prediction was correct
                SHCT[triggering_cpu][SHCT_idx]++;

                // adjust frequency tracking
                if (frequency_table[triggering_cpu][SHCT_idx].value() > 0)
                    frequency_table[triggering_cpu][SHCT_idx]--;
            }

            // insert new entry
            match->valid = true;
            match->address = full_addr;
            match->ip = ip;
            match->used = false;
            match->frequency = 1;
            match->last_type = type;
        }

        // update LRU state
        match->last_used = access_count++;
    }

    // update frequency counter for the actual cache line
    if (hit) {
        frequency_counters[freq_idx] = std::min(static_cast<int>(FREQUENCY_MAX),
                                               static_cast<int>(frequency_counters[freq_idx] + 1));
    } else {
        frequency_counters[freq_idx] = 1; // initialize frequency
    }

    // determine RRPV based on multiple factors
    if (hit) {
        // hit - cache line has high priority
        get_rrpv(set, way) = 0;
    } else {
        // miss - use prediction from SHCT
        auto SHCT_idx = get_signature(ip);
        auto predict_value = SHCT[triggering_cpu][SHCT_idx].value();
        auto freq_value = frequency_table[triggering_cpu][SHCT_idx].value();

        // calculate RRPV based on prediction and access type
        float type_weight = type_weights[triggering_cpu][static_cast<uint32_t>(type)];
        float predict_factor = static_cast<float>(predict_value) / SHCT_MAX;
        float freq_factor = static_cast<float>(freq_value) / FREQUENCY_MAX;

        // calculate initial RRPV (0 to maxRRPV)
        int initial_rrpv = static_cast<int>(predict_factor * maxRRPV);

        // apply corrections based on frequency and access type
        float correction = (1.0f - freq_factor) * 2 * type_weight;
        int final_rrpv = initial_rrpv - static_cast<int>(correction);

        // normalize
        get_rrpv(set, way) = std::min(maxRRPV - 1, std::max(0, final_rrpv));
    }
}

void eship::replacement_final_stats() {
    std::cout << "ESHIP Replacement Policy Statistics:" << std::endl;
    for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
        std::cout << "CPU " << cpu << " Hit Ratio: " << hit_ratio[cpu] << std::endl;
        std::cout << "CPU " << cpu << " Type Weights: ";
        for (uint32_t i = 0; i < 4; i++) {
            std::cout << type_weights[cpu][i] << " ";
        }
        std::cout << std::endl;
    }
}
