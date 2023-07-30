#pragma once
#include <boost/serialization/vector.hpp>
#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template<typename Archive>
void serialize(Archive& ar, PointF& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template<typename Archive>
void serialize(Archive& ar, DogSpeed& speed, [[maybe_unused]] const unsigned version) {
    ar& speed.vx;
    ar& speed.vy;
}

}  // namespace model

namespace serialization {

class LostObjectRepr {
public:
    LostObjectRepr() = default;
    explicit LostObjectRepr(const model::LostObject& object)
        : id_(object.GetId())
        , type_(object.GetType())
        , pos_(object.GetPosition()) {}

    [[nodiscard]] model::LostObject Restore() const {
        model::LostObject object{id_, type_, pos_};
        return object;
    }

    template<typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& type_;
        ar& pos_;
    }

    const model::LostObject::Id& GetId() const noexcept {
        return id_;
    }
    const model::ObjectType& GetType() const noexcept {
        return type_;
    }
    const model::PointF& GetPosition() const noexcept {
        return pos_;
    }
private:
    model::LostObject::Id id_ = model::LostObject::Id{0u};
    model::ObjectType type_ = 0;
    model::PointF pos_ = {0.0, 0.0};
};

class DogRepr {
public:
    DogRepr() = default;
    explicit DogRepr(const model::Dog& dog)
            : id_(dog.GetId())
            , name_(dog.GetName())
            , pos_(dog.GetPosition())
            , speed_(dog.GetDogSpeed())
            , direction_(dog.GetDir())
            , score_(dog.GetScore())
            , curr_road_id_(dog.GetRoad())
            , default_speed_(dog.GetDefaultSpeed())
            {
                for (auto &obj: dog.GetBagContent())
                    bag_.emplace_back(obj);
            }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, name_, default_speed_, curr_road_id_};
        dog.SetPos(pos_);
        dog.SetPrevPos(pos_);
        dog.SetSpeed(direction_);
        dog.SetDirection(direction_);
        dog.SetScore(score_);
        for (auto& item : bag_) {
            try {
                dog.GatherLostObject(item.Restore());
            } catch (...) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& default_speed_;
        ar&* curr_road_id_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_;
    }
private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    model::PointF pos_ = {0.0, 0.0};
    model::Speed default_speed_ = 0.0;
    model::DogSpeed speed_ = {0.0, 0.0};
    model::Direction direction_ = model::Direction::NORTH;
    model::Road::Id curr_road_id_ = model::Road::Id{0u};
    model::Score score_ = 0;
    std::vector<LostObjectRepr> bag_;
};

class GameSessionTestRepr {
public:
    GameSessionTestRepr() = default;
    explicit GameSessionTestRepr(const model::GameSessionBase& session): id_(session.GetId()), map_id_(session.GetMapId()) {
        for (auto &dog: session.GetDogs())
            dogs_.emplace_back(dog);
        for (auto &object: session.GetLostObjects())
            lost_objects_.emplace_back(object);
    }

    template<typename Archive>
    void serialize(Archive &ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar&* map_id_;
        ar& dogs_;
        ar& lost_objects_;
    }

    [[nodiscard]] model::GameSessionBase Restore() const {
        model::GameSessionBase session{id_, map_id_};
        for (auto &dog: dogs_)
            session.AddNewDog(dog.Restore());
        for (auto &object: lost_objects_)
            session.AddNewObject(object.Restore());
        return session;
    }
private:
    model::GameSessionBase::Id id_ = model::GameSessionBase::Id{0u};
    model::Map::Id map_id_ = model::Map::Id{"NULL"};
    std::vector<DogRepr> dogs_;
    std::vector<LostObjectRepr> lost_objects_;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;
    explicit GameSessionRepr(const model::GameSession& session): id_(session.GetId()), map_id_(session.GetMapId()) {
        for (auto &dog: session.GetDogs())
            dogs_.emplace_back(dog);
        for (auto &object: session.GetLostObjects())
            lost_objects_.emplace_back(object);
    }

    template<typename Archive>
    void serialize(Archive &ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar&* map_id_;
        ar& dogs_;
        ar& lost_objects_;
    }

    [[nodiscard]] model::GameSessionBase Restore() const {
        model::GameSessionBase session{id_, map_id_};
        for (auto &dog: dogs_)
            session.AddNewDog(dog.Restore());
        for (auto &object: lost_objects_)
            session.AddNewObject(object.Restore());
        return session;
    }
private:
    model::GameSession::Id id_ = model::GameSession::Id{0u};
    model::Map::Id map_id_ = model::Map::Id{"NULL"};
    std::vector<DogRepr> dogs_;
    std::vector<LostObjectRepr> lost_objects_;
};

}  // namespace serialization
