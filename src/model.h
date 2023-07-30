#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <iomanip>
#include <iostream>

#include "tagged.h"
#include "loot_generator.h"
#include "collision_detector.h"

constexpr int MILLISECONDS = 1000;
constexpr int MICROSECONDS = 1000000;

namespace model {

using Dimension = int;
using Coord = Dimension;
using CoordF = double;
using Speed = CoordF;
using ObjectType = size_t;
using Capacity = size_t;
using ObjectValue = size_t;
using Score = size_t;
using Time = size_t;

constexpr double ROAD_BORDER = 0.4;

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST,
    STOP,
    NONE,
};

struct Point {
    Coord x, y;
};

class Road;

struct PointF {
    CoordF x, y;
    bool operator==(const PointF& other) const {
        return (this->x == other.x && this->y == other.y);
    }
    bool operator<=(const PointF& other) const {
        return (this->x <= other.x && this->y <= other.y);
    }
    bool operator<(const PointF& other) const {
        return (this->x < other.x && this->y < other.y);
    }
    CoordF distance_to(const PointF& other) const {
        return fabs(this->x - other.x) + fabs(this->y - other.y);
    }
    PointF() = default;
    PointF(double ix, double iy): x(ix), y(iy) {}
    PointF(const std::pair<double,double> &other): x(other.first), y(other.second) {}
    PointF(const PointF& other): x(other.x), y(other.y) {}
    bool isOnRoad(const Road& road);
};

struct DogSpeed {
    Speed vx, vy;
    DogSpeed() = default;
    DogSpeed(double ivx, double ivy): vx(ivx), vy(ivy) {}
    DogSpeed(const std::pair<double,double> &other): vx(other.first), vy(other.second) {}
    DogSpeed(const DogSpeed& other): vx(other.vx), vy(other.vy) {}
    bool operator==(const DogSpeed& other) const {
        return (vx == other.vx && vy == other.vy);
    }
};

struct DogInfo {
    std::uint32_t dog_id_;
    std::string name_;
    size_t score_, playing_time_;
    DogInfo(const std::uint32_t& dog_id, const std::string& name, const size_t& score, const size_t& playing_time):
        dog_id_(dog_id), name_(name), score_(score), playing_time_(playing_time) {}
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    using Id = util::Tagged<std::uint32_t,Road>;
    using Intersections = std::unordered_map<Coord,Road::Id>;
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x, std::uint32_t id) noexcept
        : start_{start}
        , end_{end_x, start.y}
        , id_ {id} { SetBounds(); }

    Road(VerticalTag, Point start, Coord end_y, std::uint32_t id) noexcept
        : start_{start}
        , end_{start.x, end_y}
        , id_ {id} { SetBounds(); }

    void SetBounds();

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }
    PointF GetBottomLeft() const noexcept {
        return bottom_left_;
    }

    PointF GetTopRight() const noexcept {
        return top_right_;
    }
    const Id& GetId() const noexcept {
        return id_;
    }
    Intersections& GetIntersections() {
        return intersections_;
    }
    const Intersections& FindIntersections() const noexcept {
        return intersections_;
    }
    const PointF BoundToRoad(const PointF& pos) const;
private:
    Point start_;
    Point end_;
    PointF bottom_left_;
    PointF top_right_;

    Id id_;
    Intersections intersections_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, double speed, size_t bag_capacity, std::string json_data) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , default_dog_speed_(speed)
        , bag_capacity_(bag_capacity)
        , map_json_string_(std::move(json_data)){
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const std::string& GetJsonString() const noexcept {
        return map_json_string_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    const Speed& GetDefaultDogSpeed() const noexcept {
        return default_dog_speed_;
    }

    void AddRoad(Road&& road);

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void SetLootTypes(const size_t& loot_types) {
        loot_types_ = loot_types;
    }

    const size_t& GetLootTypes() const noexcept {
        return loot_types_;
    }

    const Road* FindRoad(const Road::Id& id) const noexcept {
        if (auto it = road_id_to_index_.find(id); it != road_id_to_index_.end()) {
            return &roads_.at(it->second);
        }
        return nullptr;
    }

    const size_t& GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    void AddOffice(Office &&office);

    void AddObject(const ObjectType& type, const ObjectValue& value) {
        type_to_value_.insert({type, value});
    }

    const ObjectValue GetObjectValue(const ObjectType& type) const noexcept {
        if (auto it = type_to_value_.find(type); it != type_to_value_.end())
            return it->second;
        return 0;
    }

    PointF GetRandomPosition(const Road::Id& road_id, bool rand = true) const;
    const Road::Id GetRandomRoad(bool rand = true) const;
    void FillIntersections();

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    using RoadIdToIndex = std::unordered_map<Road::Id, size_t, util::TaggedHasher<Road::Id>>;

    Id id_;
    std::string name_;
    std::string map_json_string_;
    RoadIdToIndex road_id_to_index_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    Speed default_dog_speed_;
    Capacity bag_capacity_;
    size_t loot_types_ = 0;
    std::unordered_map<ObjectType,ObjectValue> type_to_value_;
};

class LostObject {
public:
    using Id = util::Tagged<std::uint32_t,LostObject>;
    LostObject(Id id, ObjectType type, PointF pos) noexcept: id_(id), type_(type), pos_(pos) {}
    LostObject(unsigned int id, const ObjectType& type, const PointF& pos) noexcept:
        id_(LostObject::Id{id}), type_(type), pos_(pos) {}
    const std::pair<double,double> GetPos() const noexcept {
        return {pos_.x, pos_.y};
    }
    const PointF GetPosition() const noexcept {
        return pos_;
    }
    const ObjectType GetType() const noexcept {
        return type_;
    }
    const Id GetId() const noexcept {
        return id_;
    }
    bool operator==(const LostObject& other) const {
        return ((id_ == other.id_) && (pos_ == other.pos_) && (type_ == other.type_));
    }
private:
    ObjectType type_;
    PointF pos_;
    Id id_;
};

class Dog {
public:
    using Id = util::Tagged<std::uint32_t,Dog>;
    Dog(Id id, std::string name, double speed, Road::Id road_id) noexcept: id_(id), name_(std::move(name)),
                                                        default_speed_(speed), curr_road_id_(road_id) {}
    const Id& GetId() const noexcept {
        return id_;
    }
    const std::string GetName() const noexcept {
        return name_;
    }
    void SetPos(const PointF& pos) {
        pos_.x = pos.x;
        pos_.y = pos.y;
    }
    void SetPrevPos(const PointF& pos) {
        prev_pos_.x = pos.x;
        prev_pos_.y = pos.y;
    }
    void SetDirection(const model::Direction&& dir) {
        dir_ = dir;
    }
    void SetDirection(const model::Direction& dir) {
        dir_ = dir;
    }
    void SetRoad(Road::Id road_id) {
        curr_road_id_ = road_id;
    }
    void SetScore(const Score& score) {
        score_ = score;
    }
    const Road::Id GetRoad() const noexcept {
        return curr_road_id_;
    }
    void SetSpeed(const Direction& dir, bool stop = true);
    const std::pair<double,double> GetPos() const noexcept {
        return {pos_.x, pos_.y};
    }
    const PointF& GetPosition() const noexcept {
        return pos_;
    }
    const std::pair<double,double> GetPrevPos() const noexcept {
        return {prev_pos_.x, prev_pos_.y};
    }
    const PointF& GetPreviousPosition() const noexcept {
        return prev_pos_;
    }
    const std::pair<double,double> GetSpeed() const noexcept {
        return {speed_.vx, speed_.vy};
    }
    const DogSpeed& GetDogSpeed() const noexcept {
        return speed_;
    }
    const Speed GetDefaultSpeed() const noexcept {
        return default_speed_;
    }
    const Direction& GetDir() const noexcept {
        return dir_;
    }
    const size_t& GetScore() const noexcept {
        return score_;
    }
    const size_t& GetDownTime() const noexcept {
        return down_time_;
    }
    const size_t& GetPLayingTime() const noexcept {
        return playing_time_;
    }
    const size_t BagSize() const noexcept {
        return bag_.size();
    }
    void ClearBag() noexcept {
        bag_.clear();
    }
    std::vector<std::pair<size_t,size_t>> GetBag() {
        std::vector<std::pair<size_t,size_t>> bag{};
        for (auto &obj: bag_)
            bag.emplace_back(*obj.GetId(), obj.GetType());
        return bag;
    }
    const std::vector<LostObject>& GetBagContent () const {
        return bag_;
    }
    void Move(int time_interval, const Map& map);
    void GatherLostObject(const LostObject& object) {
        bag_.emplace_back(object);
    };
    void AddPoints(const size_t& points) {
        score_ += points;
    }
    void IncrementTime(const size_t& time) {
        if (!has_moved_)
            down_time_ += time;
        else
            down_time_ = 0;
        playing_time_ += time;
    }
    bool operator==(const Dog& other) const {
        return ((id_ == other.id_) && (name_ == other.name_) && (pos_ == other.pos_) && (speed_ == other.speed_)
                && (pos_ == other.pos_) && (curr_road_id_ == other.curr_road_id_) && (dir_ == other.dir_) &&
                bag_ == other.bag_ && score_ == other.score_);
    }
private:
    Id id_;
    std::string name_;
    Direction dir_ = Direction::NORTH;
    Speed default_speed_;
    DogSpeed speed_ = {0.0, 0.0};
    Road::Id curr_road_id_;
    PointF pos_ = {0.0, 0.0};
    PointF prev_pos_ = {0.0, 0.0};
    std::vector<LostObject> bag_;
    Score score_ = 0;
    Time playing_time_ = 0, down_time_ = 0;
    bool has_moved_ = false;
};

class GameSessionBase {
public:
    using Dogs = std::vector<Dog>;
    using LostObjects = std::vector<LostObject>;
    using Id = util::Tagged<std::uint32_t,GameSessionBase>;
    void AddNewDog(const Dog& dog);
    void AddNewObject(const LostObject& object);

    GameSessionBase(Id id, Map::Id map_id) noexcept: id_(id), map_id_(map_id) {}

    const Id& GetId() const noexcept {
        return id_;
    }
    const Map::Id& GetMapId() const noexcept {
        return map_id_;
    }
    const std::size_t NumberOfPlayers() const noexcept {
        return dogs_.size();
    }
    const size_t NumberOfLostObjects() const noexcept {
        return lost_objects_.size();
    }
    const Dogs& GetDogs() const noexcept {
        return dogs_;
    }
    const LostObjects& GetLostObjects() const noexcept {
        return lost_objects_;
    }

    Dog* FindDog(const Dog::Id& id) noexcept {
        if (auto it = dog_to_index_.find(id); it != dog_to_index_.end()) {
            return &dogs_.at(it->second);
        }
        return nullptr;
    }
protected:
    using DogIdHasher = util::TaggedHasher<Dog::Id>;
    using DogToIndex = std::unordered_map<Dog::Id,size_t,DogIdHasher>;
    using ObjectIdHasher = util::TaggedHasher<LostObject::Id>;
    using ObjectToIndex = std::unordered_map<LostObject::Id,size_t,ObjectIdHasher>;

    std::vector<Dog> dogs_;
    std::vector<LostObject> lost_objects_;
    DogToIndex dog_to_index_;
    ObjectToIndex object_to_index_;

    Id id_;
    Map::Id map_id_;
    std::uint32_t curr_dog_id_ = 0;
    std::uint32_t curr_obj_id_ = 0;
};

class GameSession: public GameSessionBase {
public:
    Dog::Id AddDog(const std::string& dog_name, bool rand_pos = false);
    void AddObject(const size_t& type);

    GameSession(Id id, const Map& map, loot_gen::LootGenerator& loot_generator) noexcept: GameSessionBase(id, map.GetId()), map_(map),
    loot_generator_(loot_generator) {}

    const Map& GetMap() const  noexcept {
        return map_;
    }
    const size_t& BagCapacity() const noexcept {
        return map_.GetBagCapacity();
    }

    void SetRetirementTime(const size_t& retirement_time) {
        dog_retirement_time_ = retirement_time;
    }

    void UpdateGameState(int time_interval);
    void ProcessEvents(const std::vector<collision_detector::GatheringEvent> &events);
    std::vector<DogInfo> GetRetiredPLayers(int time_interval);
    void DeleteRetiredPlayers();
    void RemoveLostObjects();
private:
    std::unordered_set<std::string> dog_names_;
    const Map& map_;
    loot_gen::LootGenerator &loot_generator_;
    size_t dog_retirement_time_ = 60000;
};

class ItemGathererProviderGame: public collision_detector::ItemGathererProvider {
public:
    explicit ItemGathererProviderGame(GameSession& game_session): game_session_(game_session) {}
    size_t OfficesCount() const {
        return game_session_.GetMap().GetOffices().size();
    }
    size_t LostObjectsCount() const {
        return game_session_.NumberOfLostObjects();
    }
    size_t ItemsCount() const override {
        return OfficesCount() + LostObjectsCount();
    }
    size_t GatherersCount() const override {
        return game_session_.NumberOfPlayers();
    }
    collision_detector::Item GetItem(size_t idx) const override {
        if (idx < LostObjectsCount()) {
            auto obj = game_session_.GetLostObjects().at(idx);
            return {{obj.GetPos().first, obj.GetPos().second}, 0};
        }
        auto office = game_session_.GetMap().GetOffices().at(idx - LostObjectsCount());
        return {{static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)}, 0.5};
    }
    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        std::vector<Dog> dogs = game_session_.GetDogs();
        auto dog = dogs.at(idx);
        return {{dog.GetPrevPos().first, dog.GetPrevPos().second},
                {dog.GetPos().first, dog.GetPos().second},
                0.6};
    }
    GameSession& GetGameSession() {
        return game_session_;
    }
private:
    GameSession& game_session_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    using Sessions = std::vector<GameSession>;

    void AddMap(Map &map);
    Dog::Id AddDog(const std::string& dog_name, const GameSession::Id& id, bool rand_pos = false);
    GameSession::Id AddSession(const Map& map);
    void AddGameSession(const GameSessionBase& session);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }
    const Map& GetMap(size_t index) const noexcept {
        return maps_.at(index);
    }
    const Sessions& GetSessions() const noexcept {
        return sessions_;
    }
    const std::uint32_t& NumberOfGameSessions() const noexcept {
        return curr_session_id_;
    }
    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }
    Dog* FindDog(const Dog::Id& dog_id, const GameSession::Id& session_id) {
        return FindSession(session_id)->FindDog(dog_id);
    }
    GameSession* FindSession(const GameSession::Id& id) noexcept {
        if (auto it = session_id_to_index_.find(id); it != session_id_to_index_.end()) {
            return &sessions_.at(it->second);
        }
        return nullptr;
    }
    const GameSession* FindGameSessionByMap(const Map* map) const noexcept {
        for (auto& session: sessions_) {
            if (session.GetMapId() == map->GetId()) return &session;
        }
        return nullptr;
    }
    std::vector<DogInfo> UpdateGame(int time_interval);
    void SetLootGenParams(const double &period, const double &probability) {
        auto time_interval = std::chrono::milliseconds(static_cast<int>(period * 1000));
        loot_generator_ = loot_gen::LootGenerator(time_interval, probability);
    }
    void SetRetirementParams(const size_t& dog_retirement_time) {
        dog_retirement_time_ = dog_retirement_time;
    }
private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using SessionIdHasher = util::TaggedHasher<GameSession::Id>;
    using SessionToIndex = std::unordered_map<GameSession::Id, size_t, SessionIdHasher>;

    GameSession* GetSession(const GameSession::Id& id) noexcept {
        if (auto it = session_id_to_index_.find(id); it != session_id_to_index_.end()) {
            return &sessions_.at(it->second);
        }
        return nullptr;
    }

    std::vector<Map> maps_;
    std::vector<GameSession> sessions_;
    std::vector<ItemGathererProviderGame> gather_handlers_;
    MapIdToIndex map_id_to_index_;
    SessionToIndex session_id_to_index_;
    std::uint32_t curr_session_id_ = 0;
    loot_gen::LootGenerator loot_generator_;
    size_t dog_retirement_time_ = 60000;
};



}  // namespace model
