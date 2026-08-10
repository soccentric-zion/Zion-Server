#include "internal/logging/logger.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace zion::log {

namespace {

// Level ordering. Higher = more severe.
constexpr std::array<std::string_view, 5> kLevelNames{
    "debug", "info", "warn", "error", "critical"
};

int level_rank(std::string_view lv) {
    for (int i = 0; i < static_cast<int>(kLevelNames.size()); ++i) {
        if (kLevelNames[i] == lv) return i;
    }
    return 1; // default: info
}

int         g_min_rank = 1; // info
std::mutex  g_mutex;

std::string iso8601_now() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt  = system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&tt, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

// Minimal JSON string escaping (no Unicode surrogate handling needed here).
std::string json_str(std::string_view s) {
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
                // Control characters: emit as \uXXXX
                if (static_cast<unsigned char>(c) < 0x20) {
                    char esc[8];
                    std::snprintf(esc, sizeof(esc), "\\u%04x", static_cast<unsigned char>(c));
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

} // namespace

void init(std::string_view level) {
    g_min_rank = level_rank(level);
}

void write(std::string_view level, std::string_view msg, Fields fields) {
    if (level_rank(level) < g_min_rank) return;

    std::ostringstream ss;
    ss << "{\"ts\":"  << json_str(iso8601_now())
       << ",\"level\":" << json_str(level)
       << ",\"msg\":"   << json_str(msg);
    for (auto& [k, v] : fields) {
        ss << ',' << json_str(k) << ':' << json_str(v);
    }
    ss << "}\n";

    std::lock_guard<std::mutex> lk(g_mutex);
    std::cerr << ss.str();
}

void debug   (std::string_view msg, Fields f) { write("debug",    msg, f); }
void info    (std::string_view msg, Fields f) { write("info",     msg, f); }
void warn    (std::string_view msg, Fields f) { write("warn",     msg, f); }
void error   (std::string_view msg, Fields f) { write("error",    msg, f); }
void critical(std::string_view msg, Fields f) { write("critical", msg, f); }

} // namespace zion::log
