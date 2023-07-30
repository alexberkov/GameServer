#include "logger.h"

namespace logger {
void LoggerFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = rec[timestamp];
    auto data = rec[additional_data].get();
    auto message = rec[expr::message].get<std::string>();
    json::value res {{"timestamp", to_iso_extended_string(*ts)}, {"data", data}, {"message", message}};
    strm << res;
}

void InitLogging() {
    logging::add_common_attributes();
    logging::add_console_log(
            std::clog,
            keywords::format = &LoggerFormatter,
            keywords::auto_flush = true
    );
}
} // namespace logger