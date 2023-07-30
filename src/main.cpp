#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <fstream>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "ticker.h"
#include "model_serialization.h"
#include "app_serialization.h"

namespace {

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace sig = boost::signals2;

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;
using InputBinaryArchive = boost::archive::binary_iarchive;
using OutputBinaryArchive = boost::archive::binary_oarchive;

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};
std::string GetConfigFromEnv() {
    std::string db_url;
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        db_url = url;
    } else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return db_url;
}

struct Args {
    std::string config_file, static_files_root;
    int tick_period = 0;
    bool randomize_spawn_points = false;
    std::string state_file_path = "NULL";
    int save_state_period = 0;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc {"Allowed options"s};
    Args args;
    desc.add_options() ("help,h", "produce help message")
            ("tick-period,t", po::value<int>(&args.tick_period)->value_name("milliseconds"s),"set tick period")
            ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
            ("www-root,w", po::value(&args.static_files_root)->value_name("dir"s), "set static files root")
            ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions")
            ("state-file", po::value(&args.state_file_path)->value_name("file"s), "set save file path")
            ("save-state-period", po::value<int>(&args.save_state_period)->value_name("milliseconds"s), "set save period");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    if (!vm.contains("config-file"s))
        throw std::runtime_error("Config file is not specified"s);
    if (!vm.contains("www-root"s))
        throw std::runtime_error("Static dir root is not specified"s);
    return args;
}

void SerializeGameState(const model::Game &game, const std::vector<app::Player>& players, const std::string& state_file_path) {
    auto temp_path = state_file_path + "_temp";
    std::ofstream out_file{state_file_path + "_temp", std::ios::binary};
    OutputBinaryArchive output_archive{out_file};
    std::vector<serialization::GameSessionRepr> sessionReprs;
    std::vector<serialization::PlayerRepr> playerReprs;
    for (auto &session: game.GetSessions())
        sessionReprs.emplace_back(session);
    for (auto &player: players) {
        if (player.IsActive())
            playerReprs.emplace_back(player);
    }
    output_archive << sessionReprs;
    output_archive << playerReprs;
    out_file.close();
    std::filesystem::remove(state_file_path);
    std::filesystem::rename(temp_path, state_file_path);
}

void DeserializeGameState(model::Game &game, http_handler::RequestHandler& handler, const std::string& state_file_path) {
    std::ifstream in_file;
    in_file.open(state_file_path, std::ios::binary);
    if (!in_file.is_open())
        return;
    InputBinaryArchive input_archive{in_file};
    std::vector<serialization::GameSessionRepr> sessionReprs;
    std::vector<serialization::PlayerRepr> playerReprs;
    input_archive >> sessionReprs;
    input_archive >> playerReprs;
    for (auto &session: sessionReprs) {
        auto session_base = session.Restore();
        game.AddGameSession(session_base);
    }
    for (auto &player: playerReprs) {
        const auto player_base = player.Restore();
        handler.AddPlayer(player_base, *game.FindSession(player_base.GetGameSessionId()));
    }
    in_file.close();
}

template <typename Fn>
void RunWorkers(unsigned num_of_threads, const Fn& func) {
    num_of_threads = std::max(1u, num_of_threads);
    std::vector<std::thread> workers;
    workers.reserve(num_of_threads - 1);
    while (--num_of_threads) {
        workers.emplace_back(func);
    }
    func();
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        if (auto args = ParseCommandLine(argc, argv)) {
            // 1. Загружаем карту из файла и построить модель игры
            model::Game game = json_loader::LoadGame(args->config_file);
            std::string static_path{args->static_files_root};
            std::string save_path{args->state_file_path};

            // 2. Инициализируем io_context
            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc((int) num_threads);

            // 3. Создаём последовательный обработчик и обрабатываем параметры
            auto strand = net::make_strand(ioc);
            bool test_mode = true, rand_pos = args->randomize_spawn_points;
            if (args->tick_period != 0)
                test_mode = false;

            // 4. Создаем БД для игры
            auto db_url = GetConfigFromEnv();
            database::Database db {pqxx::connection(db_url)};

            auto handler = std::make_shared<http_handler::RequestHandler>
                    (game, db, std::move(static_path), strand, test_mode, rand_pos);

            // 5. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc, &game, &handler, save_path](const sys::error_code &ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    SerializeGameState(game, handler->GetPlayers(), save_path);
                    ioc.stop();
                }
            });

            // 6. Инициализируем формат логирования
            logger::InitLogging();

            // 7. Загружаем состояние игры
            if (args->state_file_path != "NULL") {
                DeserializeGameState(game, *handler, args->state_file_path);
            }

            int nof_ms_total = 0;
            sig::connection conn;
            if (!test_mode) {
                // 8. Создаем тикер для обновления состояния игры во времени и сериализации
                std::chrono::milliseconds update_period {args->tick_period};
                int save_period = args->save_state_period;
                auto handle = [&game, &handler, save_period, save_path, &nof_ms_total, &db](ticker::duration time_period) {
                    int nof_ms = static_cast<int>(time_period.count()) / MICROSECONDS;
                    auto retired_players = game.UpdateGame(nof_ms);
                    for (auto &player: retired_players)
                        handler->DeletePlayer(player.dog_id_);
                    db.SavePlayers(retired_players);
                    nof_ms_total += nof_ms;
                    if ((save_path != "NULL") && (save_period > 0) && (nof_ms_total > save_period))
                        SerializeGameState(game, handler->GetPlayers(), save_path);
                };

                auto ticker = std::make_shared<ticker::Ticker>(strand, update_period, handle);
                ticker->Start();
            } else {
                conn = handler->GetApiHandler().DoOnTick([total = 0, save_path, &game, &handler](int nof_ms) mutable {
                   total += nof_ms;
                   if (save_path != "NULL")
                       SerializeGameState(game, handler->GetPlayers(), save_path);
                });
            }

            // 9. Запускаем обработчик HTTP-запросов, делегируя их обработчику запросов
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;
            http_server::ServeHttp(ioc, {address, port}, [&handler](auto &&req, auto &&send) {
                (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

            json::value data{{"port"s,    port},
                             {"address"s, address.to_string()}};
            BOOST_LOG_TRIVIAL(info) << logger::logging::add_value(logger::additional_data, data) << "server started"s;

            // 10. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] {
                ioc.run();
            });
        }
        json::value data {{"code"s, 0}};
        BOOST_LOG_TRIVIAL(error) << logger::logging::add_value(logger::additional_data, data) << "server exited"s;
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        json::value data {{"code"s, 1}, {"exception"s, ex.what()}};
        BOOST_LOG_TRIVIAL(error)  << "server exited with error: "s << ex.what();
        return EXIT_FAILURE;
    }
}
