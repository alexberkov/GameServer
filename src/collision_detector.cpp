#include "collision_detector.h"
#include <cassert>
#include <unordered_set>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return {sq_distance, proj_ratio};
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> gathering_events{};
    for (size_t i = 0; i < provider.GatherersCount(); ++i) {
        auto gatherer = provider.GetGatherer(i);
        if (!gatherer.hasMoved())
            continue;
        for (size_t j = 0; j < provider.ItemsCount(); ++j) {
            auto item = provider.GetItem(j);
            auto collect_res = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
            if (collect_res.IsCollected(gatherer.width + item.width))
                gathering_events.emplace_back(j, i, collect_res.sq_distance, collect_res.proj_ratio);
        }
    }
    std::sort(gathering_events.begin(), gathering_events.end(), [](auto &l, auto &r) {
       return l.time < r.time;
    });
    return gathering_events;
}

void FilterGatherEvents(std::vector<GatheringEvent> &events, size_t type_delim) {
    std::unordered_set<size_t> found_items;
    for (auto ev = events.begin(); ev != events.end(); ++ev) {
        if (ev->item_id < type_delim)
            ev->type = EventType::Gather;
        else
            ev->type = EventType::Drop;
        if (ev->type == EventType::Gather && found_items.find(ev->item_id) != found_items.end())
            events.erase(ev);
    }
}

}  // namespace collision_detector