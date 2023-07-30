#include <cmath>
#include <sstream>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include "../src/collision_detector.h"

namespace Catch {
    template<>
    struct StringMaker<collision_detector::GatheringEvent> {
        static std::string convert(collision_detector::GatheringEvent const& value) {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << value.item_id << value.sq_distance << value.time << ")";
            return tmp.str();
        }
    };
}  // namespace Catch

class EventComparator {
public:
    bool operator()(const auto& l, const auto& r) {
        if (l.item_id != r.item_id ||
            l.gatherer_id != r.gatherer_id ||
            std::abs(l.sq_distance - r.sq_distance) > collision_detector::eps ||
            std::abs(l.time - r.time) > collision_detector::eps)
                return false;
        else return true;
    }
};

template <typename Range,typename Comparator>
class EqualsRangeMatcher: public Catch::Matchers::MatcherGenericBase {
public:
    EqualsRangeMatcher(const Range& range, Comparator compare): range_{range}, compare_{std::move(compare)} {}
    template <typename OtherRange>
    bool match(const OtherRange& other) const {
        using std::begin;
        using std::end;
        return std::equal(begin(range_), end(range_), begin(other), end(other), compare_);
    }

    std::string describe() const override {
        return "Equals: " + Catch::rangeToString(range_);
    }

private:
    const Range& range_;
    Comparator compare_;
};

template <typename Range,typename Comparator>
auto EqualsRange(const Range& range, Comparator compare) {
    return EqualsRangeMatcher<Range,Comparator>{range, std::forward<Comparator>(compare)};
}
namespace collision_detector {

TEST_CASE("Collision detection") {
    SECTION("No gatherers provided") {
        ItemGathererProviderTest provider {{{{1, 2}, 5.}, {{0, 0}, 5.}, {{-5, 0}, 5.}}, {}};
        auto gather_events = FindGatherEvents(provider);
        CHECK(gather_events.empty());
    }

    SECTION("No items provided") {
        ItemGathererProviderTest provider {{}, {{{1, 2}, {4, 2}, 5.}, {{0, 0}, {10, 10}, 5.}, {{-5, 0}, {10, 5}, 5.}}};
        auto gather_events = FindGatherEvents(provider);
        CHECK(gather_events.empty());
    }

    SECTION("Gatherers do not move") {
        ItemGathererProviderTest provider {{
                                                   {{0, 0}, 10.}
                                           },
                                           {
                                                   {{-5, 0}, {-5, 0}, 1.},
                                                   {{0, 0}, {0, 0}, 1.},
                                                   {{-10, 10}, {-10, 10}, 100}
                                           }};
        auto gather_events = FindGatherEvents(provider);
        CHECK(gather_events.empty());
    }

    SECTION("Multiple items and one gatherer provided") {
            ItemGathererProviderTest provider {{
                                                       {{9, 0.27}, .1},
                                                       {{8, 0.24}, .1},
                                                       {{7, 0.21}, .1},
                                                       {{6, 0.18}, .1},
                                                       {{5, 0.15}, .1},
                                                       {{4, 0.12}, .1},
                                                       {{3, 0.09}, .1},
                                                       {{2, 0.06}, .1},
                                                       {{1, 0.03}, .1},
                                                       {{0, 0.0}, .1},
                                                       {{-1, 0}, .1}
                                               }, {
                                                       {{0, 0}, {10, 0}, 0.1}
                                               }};
            auto gather_events = FindGatherEvents(provider);
            auto correct_result = std::vector<GatheringEvent> {{9, 0, 0.*0., 0.0}, {8, 0, 0.03*0.03, 0.1},
                                                               {7, 0, 0.06*0.06, 0.2}, {6, 0, 0.09*0.09, 0.3},
                                                               {5, 0, 0.12*0.12, 0.4}, {4, 0, 0.15*0.15, 0.5},
                                                               {3, 0, 0.18*0.18, 0.6}};
            CHECK_THAT(gather_events, EqualsRange(correct_result, EventComparator()));
    }

    SECTION("Multiple gatherers and one item provided") {
        ItemGathererProviderTest provider {{
                                                   {{0, 0}, 0.}
                                           },
                                           {
                                                   {{-5, 0}, {5, 0}, 1.},
                                                   {{0, 1}, {0, -1}, 1.},
                                                   {{-10, 10}, {101, -100}, 0.5},
                                                   {{-100, 100}, {10, -10}, 0.5}
                                           }};
        auto gather_events = FindGatherEvents(provider);
        CHECK(gather_events.begin()->gatherer_id == 2);
    }
}

} // namespace collision_detector