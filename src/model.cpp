#include "model.h"
#include "loot_generator.h"
#include <cmath>
#include <optional>
#include <stdexcept>

namespace model {
using namespace std::literals;

PointF Map::GetRandomPosition(const Road::Id& road_id, bool rand) const {
    auto road = roads_.at(road_id_to_index_.at(road_id));
    PointF pos{};
    if (!rand) {
        pos.x = (CoordF)road.GetStart().x;
        pos.y = (CoordF)road.GetStart().y;
        return pos;
    }
    std::random_device random_device;
    auto generate_float = [&random_device](size_t min, size_t max) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(random_device);
    };
    if (road.IsHorizontal()) {
        auto end = road.GetEnd().x, start = road.GetStart().x;
        pos.x = generate_float(std::min(start, end), std::max(start, end));
        pos.y = static_cast<double>(road.GetStart().y);
    } else {
        auto end = road.GetEnd().y, start = road.GetStart().y;
        pos.y = generate_float(std::min(start, end), std::max(start, end));
        pos.x = static_cast<double>(road.GetStart().x);
    }
    return pos;
}

const Road::Id Map::GetRandomRoad(bool rand) const {
    if (!rand) return roads_.begin()->GetId();
    std::random_device random_device;
    auto generate_int = [&random_device](size_t min, size_t max) {
        std::uniform_int_distribution<size_t> dist(min, max);
        return dist(random_device);
    };
    auto random_index = generate_int(0, roads_.size() - 1);
    return roads_.at(random_index).GetId();
}

void Map::AddOffice(Office &&office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

void Map::AddRoad(Road &&road) {
    const size_t index = roads_.size();
    if (auto [it, inserted] = road_id_to_index_.emplace(road.GetId(), index); !inserted) {
        throw std::invalid_argument("Road with id "s + std::to_string(*road.GetId()) + " already exists"s);
    } else {
        try {
            roads_.emplace_back(std::move(road));
        } catch (...) {
            road_id_to_index_.erase(it);
            throw;
        }
    }
}

void Map::FillIntersections() {
    for (auto& road: roads_) {
        for (auto& other_road: roads_) {
            if (road.IsHorizontal() && other_road.IsVertical()) {
                auto x_coord = other_road.GetStart().x;
                Coord min = std::min(road.GetStart().x, road.GetEnd().x), max = std::max(road.GetStart().x, road.GetEnd().x);
                if (x_coord >= min && x_coord <= max)
                    road.GetIntersections().emplace(x_coord, other_road.GetId());
            } else if (other_road.IsHorizontal() && road.IsVertical()) {
                auto y_coord = other_road.GetStart().y;
                Coord min = std::min(road.GetStart().y, road.GetEnd().y), max = std::max(road.GetStart().y, road.GetEnd().y);
                if (y_coord >= min && y_coord <= max)
                    road.GetIntersections().emplace(y_coord, other_road.GetId());
            }
        }
    }
}

void Game::AddMap(Map &map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

Dog::Id GameSession::AddDog(const std::string& dog_name, bool rand_pos) {
    util::Tagged<std::uint32_t,Dog> dog_id {curr_dog_id_};
    const size_t index = dogs_.size();
    if (auto [it, inserted] = dog_to_index_.emplace(dog_id, index); !inserted) {
        throw std::invalid_argument("Dog with id "s + std::to_string(*dog_id) + " already exists"s);
    } else {
        try {
            auto road_id = map_.GetRandomRoad(rand_pos);
            dogs_.emplace_back(dog_id, dog_name, map_.GetDefaultDogSpeed(), road_id);
            auto curr_dog = dogs_.at(it->second);
            curr_dog.SetPos(map_.GetRandomPosition(road_id, rand_pos));
            curr_dog.SetPrevPos({curr_dog.GetPos().first, curr_dog.GetPos().second});
            ++curr_dog_id_;
            return dog_id;
        } catch (...) {
            dog_to_index_.erase(it);
            throw;
        }
    }
}

void GameSessionBase::AddNewDog(const Dog& dog) {
    const size_t index = dogs_.size();
    util::Tagged<std::uint32_t,Dog> dog_id {*dog.GetId()};
    model::Road::Id road_id {*dog.GetId()};
    if (auto [it, inserted] = dog_to_index_.emplace(dog_id, index); !inserted) {
        throw std::invalid_argument("Dog with id "s + std::to_string(*dog_id) + " already exists"s);
    } else {
        try {
            dogs_.emplace_back(dog_id, dog.GetName(), dog.GetDefaultSpeed(), road_id);
            for (auto &obj: dog.GetBagContent())
                dogs_.at(it->second).GatherLostObject(obj);
            ++curr_dog_id_;
        } catch (...) {
            dog_to_index_.erase(it);
            throw;
        }
    }
}

void GameSession::AddObject(const size_t &type) {
    util::Tagged<std::uint32_t,LostObject> object_id {curr_obj_id_};
    const size_t index = lost_objects_.size();
    if (auto [it, inserted] = object_to_index_.emplace(object_id, index); !inserted) {
        throw std::invalid_argument("Lost Object with id "s + std::to_string(*object_id) + " already exists"s);
    } else {
        try {
            auto road_id = map_.GetRandomRoad();
            auto pos = map_.GetRandomPosition(road_id);
            lost_objects_.emplace_back(object_id, type, pos);
            ++curr_obj_id_;
        } catch (...) {
            object_to_index_.erase(it);
            throw;
        }
    }
}

void GameSessionBase::AddNewObject(const LostObject &object) {
    const size_t index = lost_objects_.size();
    model::LostObject::Id id{*object.GetId()};
    if (auto [it, inserted] = object_to_index_.emplace(id, index); !inserted) {
        throw std::invalid_argument("Lost Object with id "s + std::to_string(*id) + " already exists"s);
    } else {
        try {
            lost_objects_.emplace_back(id, object.GetType(), object.GetPosition());
            ++curr_obj_id_;
        } catch (...) {
            object_to_index_.erase(it);
            throw;
        }
    }
}

CoordF bound(const CoordF& bound, const CoordF& other_bound, const CoordF& val) {
    auto low = std::min(bound, other_bound), high = std::max(bound, other_bound);
    auto res = std::min(high, val);
    return std::max(low, res);
}

const PointF Road::BoundToRoad(const PointF &pos) const {
    return {bound(bottom_left_.x, top_right_.x, pos.x),
            bound(bottom_left_.y, top_right_.y, pos.y)};
}

void Dog::Move(int time_interval, const Map& map) {
    double time_delta = static_cast<double>(time_interval) / MILLISECONDS;
    auto x_coord = pos_.x + time_delta * speed_.vx, y_coord = pos_.y + time_delta * speed_.vy;
    PointF prev_pos {pos_.x, pos_.y};
    PointF new_pos {x_coord, y_coord};
    auto curr_road = map.FindRoad(curr_road_id_);
    auto intersections = curr_road->FindIntersections();
    std::optional<const Road*> cross_road;
    Coord closest_axis = curr_road->IsHorizontal() ?
            static_cast<Coord>(std::lround(pos_.x)) : static_cast<Coord>(std::lround(pos_.y));
    if (auto cross = intersections.find(closest_axis); cross != intersections.end())
        cross_road = map.FindRoad(cross->second);
    if (new_pos.isOnRoad(*curr_road)) SetPos(new_pos);
    else if (cross_road.has_value()) {
        if (new_pos.isOnRoad(*cross_road.value())) {
            SetPos(new_pos);
            SetRoad(cross_road.value()->GetId());
        } else {
            auto curr_road_pos = curr_road->BoundToRoad(new_pos),
                    cross_road_pos = cross_road.value()->BoundToRoad(new_pos);
            if (new_pos.distance_to(curr_road_pos) < new_pos.distance_to(cross_road_pos))
                SetPos(curr_road_pos);
            else {
                SetPos(cross_road_pos);
                SetRoad(cross_road.value()->GetId());
            }
            SetSpeed(Direction::STOP, false);
        }
    } else {
        SetPos(curr_road->BoundToRoad(new_pos));
        SetSpeed(Direction::STOP, false);
    }
}

void GameSession::UpdateGameState(int time_interval) {
    for (auto& dog: dogs_)
        dog.Move(time_interval, map_);
    auto time_interval_ms = std::chrono::milliseconds(time_interval);
    auto nof_loot = loot_generator_.Generate(time_interval_ms, NumberOfLostObjects(), NumberOfPlayers());
    for (int i = 0; i < nof_loot; i++) {
        auto obj_type = loot_gen::LootGenerator::GenerateType(map_.GetLootTypes());
        this->AddObject(obj_type);
    }
}

void GameSession::ProcessEvents(const std::vector<collision_detector::GatheringEvent>& events) {
    using collision_detector::EventType;
    for (auto& event: events) {
        if (event.gatherer_id < NumberOfPlayers()) {
            auto &dog = dogs_.at(event.gatherer_id);
            if (event.type == EventType::Gather && dog.BagSize() < BagCapacity()) {
                if (event.item_id < NumberOfLostObjects()) {
                    dog.GatherLostObject(lost_objects_.at(event.item_id));
                    auto obj_id = lost_objects_.at(event.item_id).GetId();
                    if (auto it = object_to_index_.find(obj_id); it != object_to_index_.end())
                        object_to_index_.erase(it);
                }
            } else if (event.type == EventType::Drop && dog.BagSize() > 0) {
                for (auto &obj: dog.GetBag()) {
                    auto val = GetMap().GetObjectValue(obj.second);
                    dog.AddPoints(val);
                }
                dog.ClearBag();
            }
        }
    }
    RemoveLostObjects();
}

std::vector<DogInfo> GameSession::GetRetiredPLayers(int time_interval) {
    std::vector<DogInfo> retired_players;
    for (auto &dog: dogs_) {
        dog.IncrementTime(time_interval);
        if (dog.GetDownTime() >= dog_retirement_time_) {
            retired_players.emplace_back(*dog.GetId(), dog.GetName(),
                                         dog.GetScore(), dog.GetPLayingTime());
            if (auto it = dog_to_index_.find(dog.GetId()); it != dog_to_index_.end())
                dog_to_index_.erase(it);
        }
    }
    return retired_players;
}

void GameSession::DeleteRetiredPlayers() {
    std::remove_if(dogs_.begin(), dogs_.end(), [&](const auto &item) {
       return (dog_to_index_.find(item.GetId()) == dog_to_index_.end());
    });
    for (int i = 0; i < dogs_.size(); i++) {
        if (auto it = dog_to_index_.find(dogs_[i].GetId()); it != dog_to_index_.end())
            it->second = i;
    }
}

void GameSession::RemoveLostObjects() {
    std::remove_if(lost_objects_.begin(), lost_objects_.end(), [&](const auto &item){
        return (object_to_index_.find(item.GetId()) == object_to_index_.end());
    });
    for (int i = 0; i < lost_objects_.size(); i++) {
        if (auto it = object_to_index_.find(lost_objects_[i].GetId()); it != object_to_index_.end())
            it->second = i;
    }
}

std::vector<DogInfo> Game::UpdateGame(int time_interval) {
    std::vector<DogInfo> all_retired_players;
    for (auto &session: sessions_) {
        session.UpdateGameState(time_interval);
        auto retired_players = session.GetRetiredPLayers(time_interval);
        session.DeleteRetiredPlayers();
        all_retired_players.reserve(all_retired_players.size() + distance(retired_players.begin(), retired_players.end()));
        all_retired_players.insert(all_retired_players.end(), retired_players.begin(), retired_players.end());
    }
    for (auto &gather_handler: gather_handlers_) {
        auto gather_events = collision_detector::FindGatherEvents(gather_handler);
        collision_detector::FilterGatherEvents(gather_events, gather_handler.LostObjectsCount());
        gather_handler.GetGameSession().ProcessEvents(gather_events);
    }
    return all_retired_players;
}

Dog::Id Game::AddDog(const std::string& dog_name, const GameSession::Id& id, bool rand_pos) {
    auto session = GetSession(id);
    return session->AddDog(dog_name, rand_pos);
}

void Road::SetBounds() {
    bottom_left_ = {static_cast<CoordF>(std::min(start_.x, end_.x) - ROAD_BORDER),
                    static_cast<CoordF>(static_cast<double>(std::min(start_.y, end_.y)) - ROAD_BORDER)};
    top_right_ = {static_cast<CoordF>(std::max(start_.x, end_.x) + ROAD_BORDER),
                  static_cast<CoordF>(static_cast<double>(std::max(start_.y, end_.y)) + ROAD_BORDER)};
}

bool PointF::isOnRoad(const Road &road) {
    return (*this <= road.GetTopRight() && road.GetBottomLeft() <= *this);
}

GameSessionBase::Id Game::AddSession(const Map& map) {
    const size_t index = sessions_.size();
    util::Tagged<std::uint32_t,GameSessionBase> session_id {curr_session_id_};
    if (auto [it, inserted] = session_id_to_index_.emplace(curr_session_id_, index); !inserted) {
        throw std::invalid_argument("Session with id "s + std::to_string(*session_id) + " already exists"s);
    } else {
        try {
            sessions_.emplace_back(session_id, map, loot_generator_);
            gather_handlers_.emplace_back(sessions_.at(it->second));
            sessions_.at(it->second).SetRetirementTime(dog_retirement_time_);
        } catch (...) {
            session_id_to_index_.erase(it);
            throw;
        }
        ++curr_session_id_;
        return session_id;
    }
}

void Game::AddGameSession(const GameSessionBase &session) {
    const size_t index = sessions_.size();
    if (auto [it, inserted] = session_id_to_index_.emplace(session.GetId(), index); !inserted) {
        throw std::invalid_argument("Session with id "s + std::to_string(*session.GetId()) + " already exists"s);
    } else {
        try {
            auto map_index = map_id_to_index_.find(session.GetMapId())->second;
            sessions_.emplace_back(session.GetId(), GetMap(map_index), loot_generator_);
            for (auto &dog: session.GetDogs())
                sessions_.at(it->second).AddNewDog(dog);
            for (auto &object: session.GetLostObjects())
                sessions_.at(it->second).AddNewObject(object);
            gather_handlers_.emplace_back(sessions_.at(it->second));
            sessions_.at(it->second).SetRetirementTime(dog_retirement_time_);
            ++curr_session_id_;
        } catch (...) {
            session_id_to_index_.erase(it);
            throw;
        }
    }
}

void Dog::SetSpeed(const Direction &dir, bool stop) {
    switch (dir) {
        case Direction::NORTH:
            speed_.vx = 0.0;
            speed_.vy = -default_speed_;
            has_moved_ = true;
            break;
        case Direction::SOUTH:
            speed_.vx = 0.0;
            speed_.vy = default_speed_;
            has_moved_ = true;
            break;
        case Direction::WEST:
            speed_.vx = -default_speed_;
            speed_.vy = 0.0;
            has_moved_ = true;
            break;
        case Direction::EAST:
            speed_.vx = default_speed_;
            speed_.vy = 0.0;
            has_moved_ = true;
            break;
        case Direction::STOP:
            speed_.vx = 0.0;
            speed_.vy = 0.0;
            if (stop) has_moved_ = false;
        default:
            break;
    }
    if (dir != Direction::STOP && dir != Direction::NONE)
        dir_ = dir;
}

}  // namespace model
