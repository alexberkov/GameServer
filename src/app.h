#pragma once
#include <sstream>
#include "model.h"

namespace app {

using namespace std::literals;

class TokenGenerator {
private:
    std::random_device random_device_;
    std::mt19937_64 generator1_ {[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_ {[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
public:
    std::string getToken();
private:
    static constexpr size_t fmt_mult = 2;
};

class PlayerBase {
public:
    PlayerBase(const model::Dog::Id dog_id, const model::GameSession::Id session_id) noexcept:
        dog_id_(dog_id),
        game_session_id_(session_id) {}
    PlayerBase(const model::Dog::Id dog_id, const model::GameSession::Id session_id, const std::string& player_token) noexcept:
            dog_id_(dog_id),
            game_session_id_(session_id),
            player_token_(player_token) {}
    const model::Dog::Id& GetDogId() const noexcept {
        return dog_id_;
    }
    const model::GameSession::Id& GetGameSessionId() const noexcept {
        return game_session_id_;
    }
    const std::string GetToken() const noexcept {
        return player_token_;
    }
    void setToken(const std::string& token) {
        player_token_ = token;
    }
    void Deactivate() noexcept {
        active_ = false;
    }
    bool IsActive() const noexcept {
        return active_;
    }
protected:
    const model::Dog::Id dog_id_;
    const model::GameSession::Id game_session_id_;
    std::string player_token_;
    bool active_ = true;
};

class Player: public PlayerBase {
public:

    const model::GameSession::Id& GetSessionId() const noexcept {
        return session_.GetId();
    }

    void setDogSpeed(const model::Direction& dir) {
        session_.FindDog(dog_id_)->SetSpeed(dir);
    }
    Player(model::GameSession& session, const model::Dog::Id dog_id) noexcept:
        PlayerBase(dog_id, session.GetId()),
        session_(session) {}
    Player(model::GameSession& session, const PlayerBase& other) noexcept:
            PlayerBase(other.GetDogId(), session.GetId()),
            session_(session) {}
    Player(const Player& other) noexcept:
            PlayerBase(other.dog_id_, other.game_session_id_, other.player_token_),
            session_(other.session_) {}
private:
    model::GameSession& session_;
};

class Players {
public:
    std::pair<Player*,std::string> AddPlayer(const model::Dog::Id dog_id, model::GameSession& session);
    void AddPlayer(const PlayerBase& player, model::GameSession& session);
    Player* FindByToken(const std::string& token) {
        if (auto it = token_to_index_.find(token); it != token_to_index_.end()) {
            return &players_.at(it->second);
        }
        return nullptr;
    }
    const std::vector<Player>& GetPlayers() const noexcept {
        return players_;
    }
    void DeletePLayer(const std::uint32_t& dog_id);
private:
    using TokenToIndex = std::unordered_map<std::string,size_t>;
    using DogIdHasher = util::TaggedHasher<model::Dog::Id>;
    using DogToIndex = std::unordered_map<model::Dog::Id,size_t,DogIdHasher>;

    std::vector<Player> players_;
    TokenGenerator generator_;
    TokenToIndex token_to_index_;
    DogToIndex dog_to_index_;
};

} // namespace app
