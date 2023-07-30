#include "request_handler.h"
#include <map>

namespace http_handler {
std::unordered_map<std::string,std::string> ExtToContent = {
        {".htm", "text/html"}, {".html", "text/html"}, {".css", "text/css"},
        {".txt", "text/plain"}, {".js", "text/javascript"}, {".json", "application/json"},
        {".xml", "application/xml"}, {".png", "image/png"}, {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".gif", "image/gif"},
        {".bmp", "image/bmp"}, {".ico", "image/vnd.microsoft.icon"}, {".tiff", "image/tiff"},
        {".tif", "image/tiff"}, {".svg", "image/svg+xml"}, {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
};

std::string GetContentType(const char* filepath) {
    auto ext = fs::path(filepath).extension();
    std::string ext_string {ext};
    if (auto pos = ExtToContent.find(ext_string); pos != ExtToContent.end())
        return pos->second;
    return "application/octet-stream";
}

std::variant<FileResponse,StringResponse> MakeFileResponse(unsigned http_version, bool keep_alive, const char* filepath) {
    http::file_body::value_type file;

    if (sys::error_code ec; file.open(filepath, beast::file_mode::read, ec), ec) {
        StringResponse response(http::status::not_found, http_version);
        response.set(http::field::content_type, "text/plain");
        std::string_view body = "File not found"sv;
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    FileResponse response(http::status::ok, http_version);
    std::string content_type = GetContentType(filepath);
    response.set(http::field::content_type, content_type);
    response.body() = std::move(file);
    response.prepare_payload();
    return response;
}

std::optional<std::string> DecodeTarget(std::string_view req_target) {
    auto decode_res = url::make_pct_string_view(req_target);
    if (decode_res.has_value()) {
        url::pct_string_view res = decode_res.value();
        std::string target_string;
        target_string.resize(res.decoded_size());
        res.decode({}, url::string_token::assign_to(target_string));
        return target_string;
    }
    return std::nullopt;
}

bool IsSubPath(fs::path path, fs::path base) {
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) return false;
    }
    return true;
}

std::variant<FileResponse,StringResponse> RequestHandler::HandleFileRequest(StringRequest &&req) {
    const auto file_response = [&req](const char* filepath) {
        return MakeFileResponse(req.version(), req.keep_alive(), filepath);
    };

    const auto text_response = [&req](http::status status, std::string_view text) {
        return api_handler::MakeStringResponse(status, text, req.version(),
                                  req.keep_alive(), BasicLiterals::TextPlain, BasicLiterals::AllowGet);
    };

    auto method = req.method();
    std::string target {req.target()};
    if (method != http::verb::get)
        return text_response(http::status::method_not_allowed, ResponseLiterals::InvalidMethod);

    auto decoded_target = DecodeTarget(target);
    if (decoded_target.has_value()) {
        std::string base {static_dir_path_}, rel = decoded_target.value();
        if (rel == "/")
            rel += "index.html";
        std::string real_path = base + rel;
        if (IsSubPath(real_path, static_dir_path_))
            return file_response(real_path.c_str());
        return text_response(http::status::bad_request, "Invalid request URL");
    }
    return text_response(http::status::bad_request, "Incorrect request URL");
}

StringResponse RequestHandler::ReportServerError(unsigned version, bool keep_alive) {
    StringResponse response(http::status::internal_server_error, version);
    std::string_view body = "Internal server error"sv;
    response.set(http::field::content_type, "text/plain");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}
}  // namespace http_handler
