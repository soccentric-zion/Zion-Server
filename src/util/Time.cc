#include "util/Time.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace zion::util {

std::string utc_now_iso8601() {
    return to_iso8601(Clock::now());
}

int64_t utc_now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now().time_since_epoch()).count();
}

std::string to_iso8601(TimePoint tp) {
    auto t = Clock::to_time_t(tp);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count() % 1000000;
    std::tm tm{};
    gmtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    char out[48];
    std::snprintf(out, sizeof(out), "%s.%06lldZ", buf, static_cast<long long>(us));
    return out;
}

TimePoint parse_iso8601(const std::string& s) {
    std::tm tm{};
    std::istringstream ss{s};
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) return Clock::from_time_t(0);
    return Clock::from_time_t(std::mktime(&tm));
}

}  // namespace zion::util
