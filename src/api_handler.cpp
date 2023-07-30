#include "api_handler.h"
#include <iostream>
#include <list>


namespace api_handler {

using PlayerInfo = std::pair<std::string,std::string>;

std::string DirToStr(const model::Direction& dir) {
    switch (dir) {
        case model::Direction::NORTH:
            return "U";
        case model::Direction::SOUTH:
            return "D";
        case model::Direction::WEST:
            return "L";
        case model::Direction::EAST:
            return "R";
        case model::Direction::STOP:
            return "";
        default:
            return "NULL";
    }
}

model::Direction StrToDir(const std::string& dir_str) {
    if (dir_str.empty())
        return model::Direction::STOP;
    else if (dir_str == "U")
        return model::Direction::NORTH;
    else if (dir_str == "D")
        return model::Direction::SOUTH;
    else if (dir_str == "L")
        return model::Direction::WEST;
    else if (dir_str == "R")
        return model::Direction::EAST;
    else
        return model::Direction::NONE;
}

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, std::string_view content_type, const std::string_view& allowed_methods) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    response.set(http::field::cache_control, "no-cache");
    response.set(http::field::allow, allowed_methods);
    return response;
}

const model::Map* ApiHandler::GetMap(const std::string &req_target) {
    auto mapIdStr = req_target.substr(req_target.rfind('/') + 1);
    model::Map::Id mapId {mapIdStr};
    return game_.FindMap(mapId);
}

std::string ApiHandler::GetMapsJson() {
    auto maps = game_.GetMaps();
    json::array maps_json;
    for (const auto &map: maps) {
        json::object map_json;
        map_json.emplace("id", *map.GetId());
        map_json.emplace("name", map.GetName());
        maps_json.push_back(map_json);
    }
    return json::serialize(maps_json);
}

std::pair<int,int> GetRecordsParams(const std::string& target) {
    std::string_view req_target{target};
    url::url_view target_url_view(req_target);
    auto params = target_url_view.params();
    int start = 0, size = 100;
    if (auto p = params.find("start"); p != params.end())
        start = std::stoi((*p).value);
    if (auto p = params.find("maxItems"); p != params.end())
        size = std::stoi((*p).value);
    return {start, size};
}

std::string CreatePlayerInfo(std::pair<const app::Player*,std::string> info) {
    json::object playerInfo;
    std::string player_token = info.second;
    playerInfo.emplace("authToken", player_token);
    playerInfo.emplace("playerId", *info.first->GetDogId());
    return json::serialize(playerInfo);
}

std::string ApiHandler::GetPlayersInfo() {
    json::object players_info;
    auto dogs = game_.GetSessions().begin()->GetDogs();
    for (auto &dog: dogs) {
        json::object player_info;
        player_info.emplace("name", dog.GetName());
        players_info.emplace(std::to_string(*dog.GetId()), player_info);
    }
    return json::serialize(players_info);
}

json::object LoadBagItem(std::pair<size_t,size_t> bag_item) {
    json::object obj_json;
    obj_json.emplace("id", bag_item.first);
    obj_json.emplace("type", bag_item.second);
    return obj_json;
}

json::object LoadPlayer(model::Dog& dog) {
    json::object dog_info;
    json::array pos_json, speed_json, bag_json;
    auto pos = dog.GetPos(), speed = dog.GetSpeed();
    std::string dir_str = DirToStr(dog.GetDir());
    pos_json.emplace_back(pos.first);
    pos_json.emplace_back(pos.second);
    speed_json.emplace_back(speed.first);
    speed_json.emplace_back(speed.second);

    auto bag = dog.GetBag();
    for (auto &obj: bag)
        bag_json.emplace_back(LoadBagItem(obj));

    auto score = dog.GetScore();

    dog_info.emplace("pos", pos_json);
    dog_info.emplace("speed", speed_json);
    dog_info.emplace("dir", dir_str);
    dog_info.emplace("bag", bag_json);
    dog_info.emplace("score", score);

    return dog_info;
}

json::object LoadLostObject(model::LostObject& lost_object) {
    json::object object_info;
    json::array pos_json;
    auto pos = lost_object.GetPos();
    auto obj_type = lost_object.GetType();
    pos_json.emplace_back(pos.first);
    pos_json.emplace_back(pos.second);
    object_info.emplace("type", obj_type);
    object_info.emplace("pos", pos_json);
    return object_info;
}

std::string ApiHandler::GetGameState() {
    json::object game_state;
    json::object dogs_json;
    json::object lost_objects_json;

    auto dogs = game_.GetSessions().begin()->GetDogs();
    for (auto &dog: dogs)
        dogs_json.emplace(std::to_string(*dog.GetId()), LoadPlayer(dog));

    auto lost_objects = game_.GetSessions().begin()->GetLostObjects();
    for (auto &lost_object: lost_objects)
        lost_objects_json.emplace(std::to_string(*lost_object.GetId()), LoadLostObject(lost_object));

    game_state.emplace("players", dogs_json);
    game_state.emplace("lostObjects", lost_objects_json);
    return json::serialize(game_state);
}

std::string ApiHandler::GetPlayerRecords(int start, int size) {
    json::array records_json;
    auto records = db_.GetPlayers(start, size);
    for (auto &record: records) {
        json::object record_json;
        record_json.emplace("name", record.name_);
        record_json.emplace("score", record.score_);
        auto playTime = static_cast<double>(record.playing_time_) / MILLISECONDS;
        record_json.emplace("playTime", playTime);
        records_json.emplace_back(record_json);
    }
    return json::serialize(records_json);
}

AuthorizationResponse ValidateJoinRequest(const StringRequest&& req, PlayerInfo& info) {
    if (req.method() != http::verb::post)
        return AuthorizationResponse::InvalidMethod;
    bool header = false;
    for (auto &el: req.base()) {
        if ((el.name_string() == "content-type" || el.name_string() == "Content-Type")
            && el.value() == "application/json") header = true;
    }
    if (!header)
        return AuthorizationResponse::InvalidHeader;
    auto body = json::parse(req.body());
    if (body.is_null() ||
        !body.is_object() ||
        !(body.as_object().contains("userName")) ||
        !(body.as_object().contains("mapId")))
            return AuthorizationResponse::ParsingError;
    info.first = value_to<std::string>(body.as_object().at("userName"));
    if (info.first.empty())
        return AuthorizationResponse::InvalidPlayerName;
    info.second = value_to<std::string>(body.as_object().at("mapId"));
    return AuthorizationResponse::OK;
}

AuthenticationResponse ValidateAuthenticationRequest(const StringRequest&& req, std::string& token) {
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return AuthenticationResponse::InvalidMethod;
    bool auth_header = false;
    for (auto &el: req.base()) {
        if (el.name_string() == "authorization" || el.name_string() == "Authorization") {
            auth_header = true;
            token = el.value();
        }
    }
    if (!auth_header)
        return AuthenticationResponse::InvalidHeader;
    if (!token.starts_with("Bearer") ||
        token.length() != (HeaderValues::KeyLength + HeaderValues::TokenLength))
            return AuthenticationResponse::InvalidToken;
    token = token.substr(HeaderValues::KeyLength);
    return AuthenticationResponse::OK;
}

AuthenticationResponse ValidateMoveRequest(const StringRequest&& req, std::string& token) {
    if (req.method() != http::verb::post)
        return AuthenticationResponse::InvalidMethod;
    bool auth_header = false, content_header = false;
    for (auto &el: req.base()) {
        if (el.name_string() == "authorization" || el.name_string() == "Authorization") {
            auth_header = true;
            token = el.value();
        }
        if ((el.name_string() == "content-type" ||
            el.name_string() == "Content-Type") && (el.value() == "application/json"))
                content_header = true;
    }
    if (!auth_header)
        return AuthenticationResponse::InvalidHeader;
    if (!content_header)
        return AuthenticationResponse::InvalidContent;
    if (!token.starts_with("Bearer") ||
        token.length() != (HeaderValues::KeyLength + HeaderValues::TokenLength))
            return AuthenticationResponse::InvalidToken;
    token = token.substr(HeaderValues::KeyLength);
    return AuthenticationResponse::OK;
}

std::pair<ParsingResponse,model::Direction> ParseMoveRequest(const std::string& req_body) {
    auto body = json::parse(req_body);
    if (body.is_null() ||
        !body.is_object() ||
        !(body.as_object().contains("move")))
            return {ParsingResponse::ParsingError, model::Direction::NONE};
    model::Direction dir = StrToDir(value_to<std::string>(body.as_object().at("move")));
    if (dir == model::Direction::NONE)
        return {ParsingResponse::ParsingError, model::Direction::NONE};
    else
        return {ParsingResponse::OK, dir};
}

std::pair<ParsingResponse,int> ParseTickRequest(const std::string& req_body) {
    if (req_body.empty())
        return {ParsingResponse::ParsingError, NULL};
    auto body = json::parse(req_body);
    if (body.is_null() ||
        !body.is_object() ||
        !(body.as_object().contains("timeDelta")))
            return {ParsingResponse::ParsingError, NULL};
    auto time_delta_value = body.as_object().at("timeDelta");
    if (!time_delta_value.is_int64())
        return {ParsingResponse::ParsingError, NULL};
    int time_delta = value_to<int>(time_delta_value);
    if (time_delta < 0)
        return {ParsingResponse::ParsingError, NULL};
    else
        return {ParsingResponse::OK, time_delta};
}

std::pair<app::Player*,std::string> ApiHandler::AddPlayer(const std::string& dog_name, const model::Map* map) {
    if (game_.NumberOfGameSessions() == 0 || game_.FindGameSessionByMap(map) == nullptr) {
        auto session_id = game_.AddSession(*map);
        auto dog_id = game_.AddDog(dog_name, session_id, rand_pos_);
        return players_.AddPlayer(dog_id, *game_.FindSession(session_id));
    }
    auto session_id = game_.FindGameSessionByMap(map)->GetId();
    auto dog_id = game_.AddDog(dog_name, session_id, rand_pos_);
    return players_.AddPlayer(dog_id, *game_.FindSession(session_id));
}

StringResponse ApiHandler::HandleMapRequests(const StringRequest &&req, const std::string& target) {
    const auto json_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(), req.keep_alive(),
                                               BasicLiterals::AppJson, BasicLiterals::AllowGet);
    };

    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return json_response(http::status::method_not_allowed, ResponseLiterals::InvalidMethod);
    if (target.starts_with(RequestLiterals::MapTarget)) {
        auto map = GetMap(target);
        if (map != nullptr)
            return json_response(http::status::ok, map->GetJsonString());
        return json_response(http::status::not_found, ResponseLiterals::MapNotFound);
    } else if (target.starts_with(RequestLiterals::MapsTarget))
        return json_response(http::status::ok, GetMapsJson());
    return json_response(http::status::bad_request, ResponseLiterals::BadRequest);
}

StringResponse ApiHandler::HandleJoinGameRequest(const StringRequest &&req) {
    const auto json_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(),
                                               req.keep_alive(), BasicLiterals::AppJson, BasicLiterals::AllowPost);
    };

    PlayerInfo info;
    auto join_valid_res = ValidateJoinRequest(std::forward<decltype(req)>(req), info);
    if (join_valid_res == AuthorizationResponse::OK) {
        util::Tagged<std::string,model::Map> mapId {info.second};
        auto map = game_.FindMap(mapId);
        if (map == nullptr)
            return json_response(http::status::not_found, ResponseLiterals::MapNotFound);
        auto dog_name = info.first;
        auto add_res = AddPlayer(dog_name, map);
        return json_response(http::status::ok, CreatePlayerInfo(add_res));
    } else if (join_valid_res == AuthorizationResponse::InvalidPlayerName)
        return json_response(http::status::bad_request, ResponseLiterals::InvalidPlayerName);
    else if (join_valid_res == AuthorizationResponse::ParsingError)
        return json_response(http::status::bad_request, ResponseLiterals::JoinParseError);
    else if (join_valid_res == AuthorizationResponse::InvalidMethod)
        return json_response(http::status::method_not_allowed, ResponseLiterals::InvalidMethod);
    else
        return json_response(http::status::bad_request, ResponseLiterals::InvalidArgument);
}

StringResponse ApiHandler::HandleGameStateRequest(const StringRequest &&req, const std::string& target) {
    const auto json_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(),req.keep_alive(),
                                               BasicLiterals::AppJson, BasicLiterals::AllowGet);
    };

    std::string token;
    auto get_valid_res = ValidateAuthenticationRequest(std::forward<decltype(req)>(req), token);
    if (get_valid_res == AuthenticationResponse::OK) {
        auto player = players_.FindByToken(token);
        if (player == nullptr)
            return json_response(http::status::unauthorized, ResponseLiterals::PlayerNotFound);
        if (target.starts_with("/api/v1/game/players"))
            return json_response(http::status::ok, GetPlayersInfo());
        else if (target.starts_with("/api/v1/game/state"))
            return json_response(http::status::ok, GetGameState());
        else
            return json_response(http::status::bad_request, ResponseLiterals::InvalidTarget);
    } else if (get_valid_res == AuthenticationResponse::InvalidMethod)
        return json_response(http::status::method_not_allowed, ResponseLiterals::InvalidMethod);
    else
        return json_response(http::status::unauthorized, ResponseLiterals::InvalidToken);
}

StringResponse ApiHandler::HandleActionRequest(const StringRequest &&req) {
    const auto json_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(),req.keep_alive(),
                                               BasicLiterals::AppJson, BasicLiterals::AllowPost);
    };

    std::string token, body {req.body()};
    auto valid_res = ValidateMoveRequest(std::forward<decltype(req)>(req), token);
    if (valid_res == AuthenticationResponse::OK) {
        auto move_res = ParseMoveRequest(body);
        if (move_res.first == ParsingResponse::OK) {
            auto player = players_.FindByToken(token);
            if (player == nullptr)
                return json_response(http::status::unauthorized, ResponseLiterals::PlayerNotFound);
            players_.FindByToken(token)->setDogSpeed(move_res.second);
            return json_response(http::status::ok, ResponseLiterals::OK);
        } else
            return json_response(http::status::bad_request, ResponseLiterals::MoveParseError);
    } else if (valid_res == AuthenticationResponse::InvalidMethod)
        return json_response(http::status::method_not_allowed, ResponseLiterals::InvalidMethod);
    else if (valid_res == AuthenticationResponse::InvalidContent)
        return json_response(http::status::bad_request, ResponseLiterals::InvalidArgument);
    else
        return json_response(http::status::unauthorized, ResponseLiterals::InvalidToken);
}

StringResponse ApiHandler::HandleTickRequest(const StringRequest &&req) {
    const auto json_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(),req.keep_alive(),
                                               BasicLiterals::AppJson, BasicLiterals::AllowPost);
    };

    std::string body {req.body()};
    if (req.method() != http::verb::post)
        return json_response(http::status::method_not_allowed, ResponseLiterals::InvalidMethod);
    auto tick_res = ParseTickRequest(body);
    if (tick_res.first == ParsingResponse::OK) {
        auto retired_players = game_.UpdateGame(tick_res.second);
        for (auto &player: retired_players)
            players_.DeletePLayer(player.dog_id_);
        db_.SavePlayers(retired_players);
        tick_signal_(tick_res.second);
        return json_response(http::status::ok, ResponseLiterals::OK);
    } else
        return json_response(http::status::bad_request, ResponseLiterals::TickParseError);
}

StringResponse ApiHandler::HandleRecordsRequest(const StringRequest &&req, const std::string &target) {
    const auto json_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(),req.keep_alive(),
                                               BasicLiterals::AppJson, BasicLiterals::AllowGet);
    };
    auto rec_params = GetRecordsParams(target);
    if (rec_params.second > MAX_RECORDS)
        return json_response(http::status::bad_request, ResponseLiterals::InvalidSize);
    return json_response(http::status::ok, GetPlayerRecords(rec_params.first, rec_params.second));
}

StringResponse ApiHandler::HandleApiRequest(const StringRequest&& req) {
    const auto json_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(),req.keep_alive(),
                                               BasicLiterals::AppJson, BasicLiterals::AllowAll);
    };

    std::string target {req.target()};
    if (target.starts_with(RequestLiterals::JoinTarget))
        return HandleJoinGameRequest(std::forward<decltype(req)>(req));
    else if (target.starts_with(RequestLiterals::MoveTarget))
        return HandleActionRequest(std::forward<decltype(req)>(req));
    else if (target.starts_with(RequestLiterals::PlayersTarget) ||
                target.starts_with(RequestLiterals::GameStateTarget))
        return HandleGameStateRequest(std::forward<decltype(req)>(req), target);
    else if (test_mode_ && target.starts_with(RequestLiterals::TickTarget))
        return HandleTickRequest(std::forward<decltype(req)>(req));
    else if (target.starts_with(RequestLiterals::RecordsTarget))
        return HandleRecordsRequest(std::forward<decltype(req)>(req), target);
    else if (target.starts_with(RequestLiterals::MapsTarget))
        return HandleMapRequests(std::forward<decltype(req)>(req), target);
    else
        return json_response(http::status::bad_request, ResponseLiterals::InvalidTarget);
}

} // namespace api_handler
