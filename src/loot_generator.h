#pragma once
#include <chrono>
#include <functional>
#include <random>

namespace loot_gen {

class LootGenerator {
public:
    using RandomGenerator = std::function<double()>;
    using TimeInterval = std::chrono::milliseconds;
    LootGenerator() = default;
    LootGenerator(TimeInterval base_interval, double probability, RandomGenerator random_gen = DefaultGenerator)
        : base_interval_{base_interval}, probability_{probability}, random_generator_{std::move(random_gen)} { }
    unsigned Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count);
    static unsigned GenerateType(const size_t& loot_types);
private:
    static double BasicGenerator() {
        std::random_device random_device;
        std::uniform_real_distribution<double> dist(0, 1);
        return dist(random_device);
    };
    static double DefaultGenerator() noexcept {
        return 1.0;
    }
    TimeInterval base_interval_ = std::chrono::milliseconds(1000);
    double probability_ = 1.0;
    TimeInterval time_without_loot_{};
    RandomGenerator random_generator_;
};

}  // namespace loot_gen
