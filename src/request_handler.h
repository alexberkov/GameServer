#pragma once
#include "http_server.h"
#include "api_handler.h"
#include <filesystem>
#include <optional>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace url = boost::urls;
namespace sys = boost::system;
namespace net = boost::asio;
namespace json = boost::json;

using namespace std::literals;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using time_point = std::chrono::system_clock::time_point;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html";
    constexpr static std::string_view APP_JSON = "application/json";
};

template <typename Response>
Response&& LogResponse(Response&& resp, time_point start) {
    auto end = std::chrono::system_clock::now();
    int duration = static_cast<int>((end - start).count() / 1000);

    std::optional<std::string> content_type;
    for (auto &el: resp.base()) {
        if (el.name_string() == "Content-Type") {
            content_type = el.value();
            break;
        }
    }
    auto content = content_type.has_value() ? content_type.value() : "null";

    json::value data {{"response_time"s, duration}, {"code"s, resp.result_int()}, {"content_type"s, content}};
    BOOST_LOG_TRIVIAL(info) << logger::logging::add_value(logger::additional_data, data) << "response sent"s;
    return std::forward<Response>(resp);
}


class RequestHandler: public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    RequestHandler(model::Game& game, database::Database& db, std::string&& static_dir_path, Strand& api_strand, bool test = false, bool rand = false)
        : api_handler_{game, db, test, rand},
        static_dir_path_(std::forward<std::string>(static_dir_path)), api_strand_(api_strand) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    std::variant<FileResponse,StringResponse> HandleFileRequest(StringRequest&& req);
    static StringResponse ReportServerError(unsigned version, bool keep_alive);
    const std::vector<app::Player>& GetPlayers() const noexcept {
        return api_handler_.GetPlayers();
    }
    void DeletePlayer(const std::uint32_t& dog_id) {
        api_handler_.DeletePlayer(dog_id);
    }
    void AddPlayer(const app::PlayerBase& player, model::GameSession& session) {
        api_handler_.AddPlayer(player, session);
    }
    api_handler::ApiHandler& GetApiHandler() {
        return api_handler_;
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto start = std::chrono::system_clock::now();
        std::string target {req.target()};
        if (target.starts_with("/api/")) {
            auto version = req.version();
            auto keep_alive = req.keep_alive();
            auto handle = [self = shared_from_this(), send,
                    req = std::forward<decltype(req)>(req), version, keep_alive, start] {
                try {
                    assert(self->api_strand_.running_in_this_thread());
                    return send(LogResponse(self->api_handler_.HandleApiRequest(const_cast<const StringRequest&&>(req)), start));
                } catch (...) {
                    send(LogResponse(self->ReportServerError(version, keep_alive), start));
                }
            };
            return net::dispatch(api_strand_, handle);
        }
        else {
            auto res = HandleFileRequest(std::forward<decltype(req)>(req));
            if (holds_alternative<StringResponse>(res))
                send(LogResponse(std::get<StringResponse>(res), start));
            else
                send(LogResponse(std::get<FileResponse>(res), start));
        }
    }

private:
    api_handler::ApiHandler api_handler_;
    fs::path static_dir_path_;
    Strand& api_strand_;
};

}  // namespace http_handler
