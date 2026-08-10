#include "util/ResultConvert.h"
#include "repositories/ResultRepository.h"
#include "util/Time.h"

#include <json/json.h>
#include <sstream>
#include <algorithm>

namespace zion::util {

// ── Enum string helpers ──────────────────────────────────────────────────────

std::string status_to_string(int16_t status) {
    switch (status) {
    case 1: return "PASS";
    case 2: return "FAIL";
    case 3: return "ERROR";
    case 4: return "TIMEOUT";
    case 5: return "ABORTED";
    case 6: return "PARTIAL";   // deprecated alias — display only
    case 7: return "SKIPPED";
    default: return "UNSPECIFIED";
    }
}

std::string severity_to_string(int16_t severity) {
    switch (severity) {
    case 1: return "INFO";
    case 2: return "WARNING";
    case 3: return "ERROR";
    case 4: return "CRITICAL";
    default: return "UNSPECIFIED";
    }
}

int16_t string_to_status(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "PASS")    return 1;
    if (upper == "FAIL")    return 2;
    if (upper == "ERROR")   return 3;
    if (upper == "TIMEOUT") return 4;
    if (upper == "ABORTED") return 5;
    if (upper == "SKIPPED") return 7;
    return 0;
}

int16_t string_to_severity(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "INFO")     return 1;
    if (upper == "WARNING")  return 2;
    if (upper == "ERROR")    return 3;
    if (upper == "CRITICAL") return 4;
    return 0;
}

// ── TypedValue JSON helpers ──────────────────────────────────────────────────

static Json::Value typed_value_to_json(const hal::common::TypedValue& tv) {
    Json::Value obj;
    switch (tv.value_case()) {
    case hal::common::TypedValue::kBoolValue:
        obj["type"]  = "bool";
        obj["value"] = tv.bool_value();
        break;
    case hal::common::TypedValue::kIntValue:
        obj["type"]  = "int";
        obj["value"] = static_cast<Json::Int64>(tv.int_value());
        break;
    case hal::common::TypedValue::kDoubleValue:
        obj["type"]  = "double";
        obj["value"] = tv.double_value();
        break;
    case hal::common::TypedValue::kStringValue:
        obj["type"]  = "string";
        obj["value"] = tv.string_value();
        break;
    default:
        // VALUE_NOT_SET — return empty object
        break;
    }
    return obj;
}

std::string typed_map_to_json(
        const google::protobuf::Map<std::string, hal::common::TypedValue>& m) {
    Json::Value root(Json::objectValue);
    for (const auto& [k, v] : m) {
        root[k] = typed_value_to_json(v);
    }
    Json::FastWriter fw;
    std::string s = fw.write(root);
    // FastWriter appends a newline — trim it.
    if (!s.empty() && s.back() == '\n') s.pop_back();
    return s;
}

std::string string_map_to_json(
        const google::protobuf::Map<std::string, std::string>& m) {
    Json::Value root(Json::objectValue);
    for (const auto& [k, v] : m) {
        root[k] = v;
    }
    Json::FastWriter fw;
    std::string s = fw.write(root);
    if (!s.empty() && s.back() == '\n') s.pop_back();
    return s;
}

Json::Value parse_typed_map_json(const std::string& json_str) {
    Json::Value root;
    if (json_str.empty() || json_str == "{}") return Json::objectValue;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream ss(json_str);
    if (!Json::parseFromStream(rb, ss, &root, &errs)) return Json::objectValue;
    return root;
}

Json::Value parse_string_map_json(const std::string& json_str) {
    Json::Value root;
    if (json_str.empty() || json_str == "{}") return Json::objectValue;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream ss(json_str);
    if (!Json::parseFromStream(rb, ss, &root, &errs)) return Json::objectValue;
    return root;
}

// ── Proto → Result conversion ────────────────────────────────────────────────

Result proto_to_result(const hal::common::TestResult& tr,
                       const std::string& run_id,
                       const std::string& module_version,
                       const std::string& mode,
                       const std::string& parent_id) {
    Result r;
    r.run_id         = run_id;
    r.module_id      = tr.module_id();
    r.module_version = module_version;
    r.mode           = mode;
    r.parent_id      = parent_id.empty() ? std::nullopt
                                         : std::optional<std::string>{parent_id};

    r.test_id   = tr.test_id();
    r.test_name = tr.name();
    r.sequence  = static_cast<int64_t>(tr.sequence());

    // Timestamp: prefer proto timestamp; fall back to now.
    if (tr.has_timestamp()) {
        auto tp = std::chrono::system_clock::time_point{
            std::chrono::seconds{tr.timestamp().seconds()} +
            std::chrono::nanoseconds{tr.timestamp().nanos()}};
        r.timestamp = util::to_iso8601(tp);
    } else {
        r.timestamp = util::utc_now_iso8601();
    }

    // Duration in nanoseconds.
    if (tr.has_duration()) {
        r.duration_ns = tr.duration().seconds() * 1'000'000'000LL
                      + tr.duration().nanos();
    }

    // Output fields.
    const auto& out  = tr.output();
    r.status         = static_cast<int16_t>(out.status());
    r.severity       = static_cast<int16_t>(out.severity());
    r.error_message  = out.error_message();
    r.reason_json    = string_map_to_json(out.reason());
    r.metrics_json   = typed_map_to_json(out.metrics());
    r.monitoring_json= typed_map_to_json(out.monitoring());
    r.event_json     = typed_map_to_json(out.event());

    // payload_type_url and payload_bytes: not set here — caller sets them if needed.
    return r;
}

}  // namespace zion::util
