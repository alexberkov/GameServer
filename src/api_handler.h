#pragma once
#include "app.h"
#include "postgres.h"
#include <boost/json.hpp>
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>
#include <boost/url.hpp>

using literal = std::string_view;

class ResponseLiterals {
public:
    static constexpr literal InvalidPlayerName = R"({"code": "invalidArgument", "message": "Invalid name"})";
    static constexpr literal InvalidMethod = R"({"code": "invalidMethod", "message": "Invalid method"})";
    static constexpr literal InvalidArgument = R"({"code": "invalidArgument", "message": "Invalid content type"})";
    static constexpr literal InvalidTarget = R"({"code": "badRequest", "message": "Target not found"})";
    static constexpr literal InvalidToken = R"({"code": "invalidToken", "message": "Authorization header is missing"})";
    static constexpr literal InvalidSize = R"({"code": "invalidArgument", "message": "Max number of records exceeded"})";
    static constexpr literal MapNotFound = R"({"code": "mapNotFound", "message": "Map not found"})";
    static constexpr literal PlayerNotFound = R"({"code": "unknownToken", "message": "Player token has not been found"})";
    static constexpr literal JoinParseError = R"({"code": "invalidArgument", "message": "Join game request parse error"})";
    static constexpr literal MoveParseError = R"({"code": "invalidArgument", "message": "Failed to parse action"})";
    static constexpr literal TickParseError = R"({"code": "invalidArgument", "message": "Failed to parse tick request JSON"})";
    static constexpr literal BadRequest = R"({"code": "badRequest", "message": "Bad request"})";
    static constexpr literal OK = R"({})";
};

class RequestLiterals {
public:
    static constexpr literal JoinTarget = "/api/v1/game/join";
    static constexpr literal MoveTarget = "/api/v1/game/player/action";
    static constexpr literal PlayersTarget = "/api/v1/game/players";
    static constexpr literal GameStateTarget = "/api/v1/game/state";
    static constexpr literal TickTarget = "/api/v1/game/tick";
    static constexpr literal RecordsTarget = "/api/v1/game/records";
    static constexpr literal MapsTarget = "/api/v1/maps";
    static constexpr literal MapTarget = "/api/v1/maps/";
};

class BasicLiterals {
public:
    static constexpr literal AppJson = "application/json";
    static constexpr literal TextPlain = "text/plain";
    static constexpr literal AllowGet = "GET, HEAD";
    static constexpr literal AllowPost = "POST";
    static constexpr literal AllowAll = "GET, HEAD, POST";
};

namespace api_handler {

namespace json = boost::json;
namespace http = boost::beast::http;
namespace sig = boost::signals2;
namespace url = boost::urls;
using StringResponse = http::response<http::string_body>;
using StringRequest = http::request<http::string_body>;
using TickSignal = sig::signal<void(int nof_ms)>;

constexpr int MAX_RECORDS = 100;

enum class AuthorizationResponse {
    OK,
    InvalidPlayerName,
    ParsingError,
    InvalidMethod,
    InvalidHeader
};

enum class AuthenticationResponse {
    OK,
    InvalidHeader,
    InvalidToken,
    InvalidMethod,
    InvalidContent
};

enum class ParsingResponse {
    OK,
    ParsingError
};

enum HeaderValues {
    KeyLength = 7,
    TokenLength = 32
};

class ApiHandler {
    const model::Map* GetMap(const std::string &req_target);
    std::string GetMapsJson();
public:
    explicit ApiHandler(model::Game& game, database::Database& db, bool test_mode = false, bool rand_pos = false):
                game_(game), db_(db),
                test_mode_(test_mode), rand_pos_(rand_pos) {}
    StringResponse HandleApiRequest(const StringRequest&& req);
    const std::vector<app::Player>& GetPlayers() const noexcept {
        return players_.GetPlayers();
    }
    void DeletePlayer(const std::uint32_t& dog_id) {
        players_.DeletePLayer(dog_id);
    }
    void AddPlayer(const app::PlayerBase& player, model::GameSession& session) {
        players_.AddPlayer(player, session);
    }
    [[nodiscard]] sig::connection DoOnTick(const TickSignal::slot_type& handler) {
        return tick_signal_.connect(handler);
    }
private:
    std::pair<app::Player*,std::string> AddPlayer(const std::string& dog_name, const model::Map* map);
    std::string GetPlayersInfo();
    std::string GetGameState();
    std::string GetPlayerRecords(int start, int size);
    StringResponse HandleJoinGameRequest(const StringRequest&& req);
    StringResponse HandleMapRequests(const StringRequest&& req, const std::string& target);
    StringResponse HandleGameStateRequest(const StringRequest&& req, const std::string& target);
    StringResponse HandleActionRequest(const StringRequest&& req);
    StringResponse HandleTickRequest(const StringRequest&& req);
    StringResponse HandleRecordsRequest(const StringRequest&& req, const std::string& target);
    model::Game& game_;
    app::Players players_;
    bool test_mode_, rand_pos_;
    TickSignal tick_signal_;
    database::Database& db_;
};


StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive, std::string_view content_type, const std::string_view& allowed_methods);

} // namespace api_handler



