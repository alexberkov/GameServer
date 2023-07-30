#include "http_server.h"
#include <boost/asio/dispatch.hpp>

namespace http_server {

    void ReportError(beast::error_code ec, std::string_view what) {
        logger::json::value data {{"code"s, ec.value()}, {"text"s, ec.what()}, {"where"s, what}};
        BOOST_LOG_TRIVIAL(error) << logger::logging::add_value(logger::additional_data, data) << "error"s;
    }

    void SessionBase::Run() {
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    void SessionBase::Read() {
        request_ = {};
        stream_.expires_after(30s);
        http::async_read(stream_, buffer_, request_,
                         beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }

    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        if (ec == http::error::end_of_stream)
            return Close();
        if (ec)
            return ReportError(ec, "read"sv);
        HandleRequest(std::move(request_));
    }

    void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        if (ec)
            return ReportError(ec, "write"sv);
        if (close)
            return Close();
        Read();
    }

    void SessionBase::Close() {
        stream_.socket().shutdown(tcp::socket::shutdown_send);
    }

    void SessionBase::LogRequest(const HttpRequest& req) {
        auto addr = stream_.socket().remote_endpoint().address().to_string();
        logger::json::value data {{"ip"s, addr}, {"URI"s, req.target()}, {"method"s, req.method_string()}};
        BOOST_LOG_TRIVIAL(info) << logger::logging::add_value(logger::additional_data, data) << "request received"s;
    }

}  // namespace http_server
