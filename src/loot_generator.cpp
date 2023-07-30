#include "loot_generator.h"

#include <algorithm>
#include <cmath>

namespace loot_gen {

unsigned LootGenerator::Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count) {
    time_without_loot_ += time_delta;
    const unsigned loot_shortage = loot_count > looter_count ? 0u : looter_count - loot_count;
    const double ratio = std::chrono::duration<double>{time_without_loot_} / base_interval_;
    const double probability = std::clamp((1.0 - std::pow(1.0 - probability_, ratio)) * random_generator_(), 0.0, 1.0);
    const auto generated_loot = static_cast<unsigned>(std::round(loot_shortage * probability));
    if (generated_loot > 0)
        time_without_loot_ = {};
    return generated_loot;
}

unsigned LootGenerator::GenerateType(const size_t &loot_types) {
    std::random_device random_device;
    std::uniform_int_distribution<unsigned> dist(0, loot_types - 1);
    return dist(random_device);
}

} // namespace loot_gen
