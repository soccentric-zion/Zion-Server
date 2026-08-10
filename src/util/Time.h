#pragma once
// util/Time.h — timestamp helpers.
#include <chrono>
#include <string>

namespace zion::util {

using Clock     = std::chrono::system_clock;
using TimePoint = Clock::time_point;

// Returns current UTC time as an ISO-8601 string: "2026-01-02T15:04:05Z".
std::string utc_now_iso8601();

// Returns current UTC time as a microseconds-since-epoch integer.
int64_t utc_now_us();

// Parse an ISO-8601 string to TimePoint.  Returns epoch on parse failure.
TimePoint parse_iso8601(const std::string& s);

// Format a TimePoint to ISO-8601 string.
std::string to_iso8601(TimePoint tp);

}  // namespace zion::util
