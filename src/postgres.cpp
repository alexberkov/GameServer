#include "postgres.h"
#include <iostream>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace database {

using UUIDType = boost::uuids::uuid;
UUIDType NewUUID() {
    return boost::uuids::random_generator()();
}

std::string UUIDToString(const UUIDType& uuid) {
    return to_string(uuid);
}

UUIDType UUIDFromString(std::string_view str) {
    boost::uuids::string_generator gen;
    return gen(str.begin(), str.end());
}

Database::Database(pqxx::connection connection): connection_{std::move(connection)} {
    try {
        pqxx::work work_table{connection_};
        work_table.exec(R"(CREATE TABLE IF NOT EXISTS retired_players
                    (id UUID CONSTRAINT player_id_constraint PRIMARY KEY,
                    name varchar(100) NOT NULL,
                    score integer NOT NULL DEFAULT 0,
                    play_time_ms integer NOT NULL DEFAULT 0);)"_zv);
        work_table.commit();

        pqxx::work work_index{connection_};
        work_index.exec(R"(CREATE INDEX IF NOT EXISTS retired_players_index
                    ON retired_players (score DESC, play_time_ms, name);)"_zv);
        work_index.commit();

        connection_.prepare(tag_ins_player, "INSERT INTO retired_players VALUES ($1, $2, $3, $4);"_zv);
        connection_.prepare(tag_get_players,
                            "SELECT * FROM retired_players ORDER BY score DESC, play_time_ms ASC, name ASC LIMIT $1 OFFSET $2;"_zv);
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        throw std::runtime_error("Could not create database");
    }
}

std::vector<model::DogInfo> Database::GetPlayers(int start, int size) {
    std::vector<model::DogInfo> players;
    pqxx::work work{connection_};
    auto res = work.exec_prepared(tag_get_players, size, start);
    for (auto row: res)
        players.emplace_back(0, to_string(row.at("name")),
                             row.at("score").as<size_t>(), row.at("play_time_ms").as<size_t>());
    return players;
}

void Database::SavePlayers(const std::vector<model::DogInfo> &retired_players) {
    pqxx::work work{connection_};
    for (auto &player: retired_players) {
        auto player_id = UUIDToString(NewUUID());
        work.exec_prepared(tag_ins_player, player_id, player.name_, player.score_, player.playing_time_);
    }
    work.commit();
}

}

