#pragma once
#include <chrono>
#include <utility>
#include <boost/beast.hpp>
#include <boost/asio/strand.hpp>

namespace ticker {
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace net = boost::asio;
using duration = net::steady_timer::duration;

class Ticker: public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(duration delta)>;
    using ms = std::chrono::milliseconds;

    Ticker(Strand& strand, ms period, Handler handler): strand_(strand), handler_(std::move(handler)),
                                                       period_(std::chrono::duration_cast<duration>(period)) {}
    void Start();

private:
    void ScheduleTick();
    void OnTick(sys::error_code ec);

    Strand& strand_;
    net::steady_timer timer_ {strand_};
    duration period_;
    decltype(std::chrono::steady_clock::now()) last_tick_;
    Handler handler_;
};
} // namespace ticker


