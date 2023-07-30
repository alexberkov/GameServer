#include "app.h"

namespace app {

std::string TokenGenerator::getToken() {
    std::stringstream res;
    res << std::setfill('0') << std::setw(sizeof(std::uint64_t) * fmt_mult) << std::hex << generator1_();
    res << std::setfill('0') << std::setw(sizeof(std::uint64_t) * fmt_mult) << std::hex << generator2_();
    return res.str();
}

std::pair<Player*,std::string> Players::AddPlayer(const model::Dog::Id dog_id, model::GameSession& session) {
    const size_t index = players_.size();
    auto player_token = generator_.getToken();
    if (auto [it, inserted] = token_to_index_.emplace(player_token, index); !inserted) {
        throw std::invalid_argument("Player with token "s + player_token + " already exists"s);
    } else {
        try {
            players_.emplace_back(session, dog_id);
            players_.at(it->second).setToken(player_token);
            dog_to_index_.emplace(dog_id, index);
        } catch (...) {
            token_to_index_.erase(it);
            throw;
        }
    }
    return {FindByToken(player_token), player_token};
}

void Players::AddPlayer(const PlayerBase& player, model::GameSession& session) {
    const size_t index = players_.size();
    auto player_token = player.GetToken();
    if (auto [it, inserted] = token_to_index_.emplace(player_token, index); !inserted) {
        throw std::invalid_argument("Player with token "s + player_token + " already exists"s);
    } else {
        try {
            players_.emplace_back(session, player.GetDogId());
            players_.at(it->second).setToken(player_token);
            dog_to_index_.emplace(player.GetDogId(), index);
        } catch (...) {
            token_to_index_.erase(it);
            throw;
        }
    }
}

void Players::DeletePLayer(const std::uint32_t& dog_id) {
    if (auto dog_pos = dog_to_index_.find(model::Dog::Id{dog_id}); dog_pos != dog_to_index_.end()) {
        auto index = dog_pos->second;
        if (index < players_.size()) {
            auto token = players_.at(index).GetToken();
            if (auto token_pos = token_to_index_.find(token); token_pos != token_to_index_.end()) {
                token_to_index_.erase(token_pos);
                dog_to_index_.erase(dog_pos);
                players_.at(index).Deactivate();
            }
        }
    }
}

} // namespace app
