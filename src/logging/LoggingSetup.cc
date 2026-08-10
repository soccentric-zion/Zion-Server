#include "logging/LoggingSetup.h"

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <unordered_map>
#include <string>

namespace zion::logging {

namespace {

// Per-thread context fields appended to every log record.
thread_local std::unordered_map<std::string, std::string> t_ctx;

// Custom formatter that emits JSON and appends thread-local context fields.
class JsonFormatter final : public spdlog::formatter {
public:
    void format(const spdlog::details::log_msg& msg,
                spdlog::memory_buf_t& dest) override {
        // Build a minimal JSON log line.
        // Format: {"ts":"...","level":"...","msg":"...","key":"val",...}\n
        using clock = std::chrono::system_clock;
        auto tp   = msg.time;
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::tm tm{};
        gmtime_r(&time, &tm);
        char ts_buf[32];
        std::strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%dT%H:%M:%SZ", &tm);

        auto level_sv = spdlog::level::to_string_view(msg.level);
        std::string level_str(level_sv.data(), level_sv.size());

        std::string payload;
        payload.reserve(256);
        payload += R"({"ts":")";
        payload += ts_buf;
        payload += R"(","level":")";
        payload += level_str;
        payload += R"(","msg":)";
        payload += json_escape({msg.payload.data(), msg.payload.size()});

        for (const auto& [k, v] : t_ctx) {
            payload += ",\"";
            payload += k;
            payload += "\":";
            payload += json_escape(v);
        }

        payload += "}\n";
        dest.append(payload.data(), payload.data() + payload.size());
    }

    [[nodiscard]] std::unique_ptr<spdlog::formatter> clone() const override {
        return std::make_unique<JsonFormatter>();
    }

private:
    static std::string json_escape(std::string_view s) {
        std::string out;
        out.reserve(s.size() + 4);
        out += '"';
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char esc[8];
                        std::snprintf(esc, sizeof(esc), "\\u%04x",
                                      static_cast<unsigned char>(c));
                        out += esc;
                    } else {
                        out += c;
                    }
                    break;
            }
        }
        out += '"';
        return out;
    }
};

}  // namespace

void setup_logging(std::string_view level) {
    spdlog::init_thread_pool(8192, 2);  // queue size, background threads

    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    stdout_sink->set_formatter(std::make_unique<JsonFormatter>());

    const char* slug = std::getenv("PLATFORM_SLUG");
    auto logger = std::make_shared<spdlog::async_logger>(
        slug != nullptr ? slug : "zion",
        stdout_sink,
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::from_str(std::string{level}));
    spdlog::flush_on(spdlog::level::err);
}

void set_context(std::string_view key, std::string_view value) {
    t_ctx[std::string{key}] = std::string{value};
}

void clear_context() {
    t_ctx.clear();
}

}  // namespace zion::logging
