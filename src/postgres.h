#pragma once
#include <pqxx/pqxx>

#include "model.h"

namespace database {

using namespace std::literals;
using pqxx::operator"" _zv;

constexpr auto tag_ins_player = "ins_player"_zv, tag_get_players = "get_players"_zv;

class Database {
public:
    explicit Database(pqxx::connection connection);
    pqxx::connection& GetConnection() {
        return connection_;
    }

    std::vector<model::DogInfo> GetPlayers(int start, int size);
    void SavePlayers(const std::vector<model::DogInfo>& retired_players);

private:
    pqxx::connection connection_;
};

} // namespace database;

