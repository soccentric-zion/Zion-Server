#pragma once
// util/ResultConvert.h — conversion helpers between hal::common::TestResult proto
// and the zion::Result DB struct, plus TypedValue JSON serialisation.

#include "common.pb.h"

#include <json/json.h>
#include <string>
#include <cstdint>

namespace zion {
// Forward-declare to avoid circular include with ResultRepository.h
struct Result;
}  // namespace zion

namespace zion::util {

// ── Enum string helpers ──────────────────────────────────────────────────────

// Returns the canonical uppercase REST string for a TestStatus integer.
// e.g. status_to_string(1) → "PASS"
// Unknown values return "UNKNOWN".
std::string status_to_string(int16_t status);

// Returns the canonical uppercase REST string for a TestSeverity integer.
// e.g. severity_to_string(1) → "INFO"
std::string severity_to_string(int16_t severity);

// Parse a status string (case-insensitive) back to an integer.
// Returns 0 (UNSPECIFIED) on unrecognised input.
int16_t string_to_status(const std::string& s);

// Parse a severity string (case-insensitive) back to an integer.
int16_t string_to_severity(const std::string& s);

// ── TypedValue JSON serialisation ────────────────────────────────────────────

// Serialise a map<string, TypedValue> to a JSONB-ready string.
// Output:  { "key": { "type": "double", "value": 42.5 }, ... }
std::string typed_map_to_json(
    const google::protobuf::Map<std::string, hal::common::TypedValue>& m);

// Serialise a map<string, string> (reason) to a JSONB-ready string.
// Output: { "key": "value", ... }
std::string string_map_to_json(
    const google::protobuf::Map<std::string, std::string>& m);

// Parse a JSONB typed-map string back to a Json::Value for REST output.
// Returns Json::objectValue on parse failure.
Json::Value parse_typed_map_json(const std::string& json_str);

// Parse a JSONB string-map string back to a Json::Value for REST output.
Json::Value parse_string_map_json(const std::string& json_str);

// ── Proto → Result conversion ────────────────────────────────────────────────

// Convert a hal::common::TestResult proto to an zion::Result DB struct.
// parent_id is empty for top-level results; set by the recursive caller for subtests.
// The caller must set result.id before inserting if the DB does not auto-generate one.
Result proto_to_result(const hal::common::TestResult& tr,
                       const std::string& run_id,
                       const std::string& module_version,
                       const std::string& mode,
                       const std::string& parent_id = "");

}  // namespace zion::util
