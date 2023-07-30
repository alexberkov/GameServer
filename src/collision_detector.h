#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>
#include <limits>

namespace collision_detector {

constexpr double eps = std::numeric_limits<double>::epsilon();

enum class EventType {
    None,
    Gather,
    Drop
};

struct CollectionResult {
    CollectionResult(const double& sq_dist, const double& proj_rat): sq_distance(sq_dist), proj_ratio(proj_rat) {}
    [[nodiscard]] bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }

    double sq_distance;
    double proj_ratio;
};

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

struct Item {
    Item(geom::Point2D pos, double w): position(pos), width(w) {}
    geom::Point2D position;
    double width{};
};

struct Gatherer {
    Gatherer(geom::Point2D start, geom::Point2D end, double w): start_pos(start), end_pos(end), width(w) {}
    bool hasMoved() const {
        return ((std::abs(start_pos.x - end_pos.x) > eps) || (std::abs(start_pos.y - end_pos.y) > eps));
    }
    geom::Point2D start_pos;
    geom::Point2D end_pos;
    double width{};
};

class ItemGathererProvider {
public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;

    virtual ~ItemGathererProvider() = default;
};

class ItemGathererProviderTest: public ItemGathererProvider {
public:
    ItemGathererProviderTest(std::vector<Item> items, std::vector<Gatherer> gatherers):
        items_(items), gatherers_(gatherers) {}
    size_t ItemsCount() const override {
        return items_.size();
    }
    Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }
    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }
    void AddGatherer(geom::Point2D start, geom::Point2D end, double w) {
        gatherers_.emplace_back(start, end, w);
    }
    void AddItem(geom::Point2D pos, double w) {
        items_.emplace_back(pos, w);
    }
private:
    std::vector<Gatherer> gatherers_;
    std::vector<Item> items_;
};

struct GatheringEvent {
    GatheringEvent(size_t _item_id, size_t _gatherer_id, double _sq_dist, double _time): item_id(_item_id),
    gatherer_id(_gatherer_id), sq_distance(_sq_dist), time(_time) {}
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
    EventType type = EventType::None;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);
void FilterGatherEvents(std::vector<GatheringEvent> &events, size_t type_delim);

}  // namespace collision_detector