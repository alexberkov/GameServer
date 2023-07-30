#pragma once
#include <boost/serialization/vector.hpp>
#include "app.h"

namespace serialization {
class PlayerTestRepr {
public:
    PlayerTestRepr() = default;
    explicit PlayerTestRepr(const app::PlayerBase& player):
            dog_id_(player.GetDogId()),
            game_session_id_(player.GetGameSessionId()),
            token_(player.GetToken()) {}
    [[nodiscard]] app::PlayerBase Restore() const {
        app::PlayerBase player{dog_id_, game_session_id_, token_};
        return player;
    }
    template<typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* dog_id_;
        ar&* game_session_id_;
        ar& token_;
    }
private:
    model::Dog::Id dog_id_ = model::Dog::Id{0u};
    model::GameSession::Id game_session_id_ = model::GameSession::Id{0u};
    std::string token_;
};

class PlayerRepr {
public:
    PlayerRepr() = default;
    explicit PlayerRepr(const app::Player& player):
            dog_id_(player.GetDogId()),
            game_session_id_(player.GetGameSessionId()),
            token_(player.GetToken()) {}
    [[nodiscard]] app::PlayerBase Restore() const {
        app::PlayerBase player{dog_id_, game_session_id_, token_};
        return player;
    }
    template<typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* dog_id_;
        ar&* game_session_id_;
        ar& token_;
    }
private:
    model::Dog::Id dog_id_ = model::Dog::Id{0u};
    model::GameSession::Id game_session_id_ = model::GameSession::Id{0u};
    std::string token_;
};
}
