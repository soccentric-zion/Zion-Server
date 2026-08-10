#pragma once
// repositories/ResultRepository.h
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace zion {

// ── Result ──────────────────────────────────────────────────────────────────
// DB projection of the results table.  All new structured fields are present.
// status / severity are stored as SMALLINT matching the TestStatus / TestSeverity
// proto enum integers (see migrations/0010_result_fields.sql for mapping table).
struct Result {
    std::string id;
    std::string run_id;
    std::string module_id;
    std::string module_version;
    std::string timestamp;   // ISO-8601 UTC
    std::string mode;

    // New structured fields (from TestResult / TestOutput).
    int16_t     status   = 0;   // TestStatus enum integer (1=PASS,2=FAIL,3=ERROR,4=TIMEOUT,5=ABORTED,7=SKIPPED)
    int16_t     severity = 0;   // TestSeverity enum integer (1=INFO,2=WARNING,3=ERROR,4=CRITICAL)
    std::string test_id;
    std::string test_name;
    int64_t     sequence    = 0;
    int64_t     duration_ns = 0;
    std::string error_message;

    // JSONB columns: stored as serialised JSON strings.
    // reason:     map<string,string>     as  { "key": "value" }
    // metrics:    map<string,TypedValue> as  { "key": {"type":"T","value":V} }
    // monitoring: map<string,TypedValue>
    // event:      map<string,TypedValue>
    std::string reason_json;
    std::string metrics_json;
    std::string monitoring_json;
    std::string event_json;

    // Subtest linkage: NULL for top-level results.
    std::optional<std::string> parent_id;

    // Raw proto payload (retained for fidelity alongside structured columns).
    std::vector<char> payload_bytes;
    std::string       payload_type_url;

    // In-memory subtests populated lazily; not a DB column.
    std::vector<Result> subtests;
};

// ── ResultFilter ─────────────────────────────────────────────────────────────
struct ResultFilter {
    std::string              run_id;
    std::string              module_id;
    std::string              test_id;
    std::vector<int16_t>     statuses;     // empty = all statuses
    std::vector<int16_t>     severities;   // empty = all severities
    std::string              since;        // ISO-8601 lower bound (inclusive)
    std::string              until;        // ISO-8601 upper bound (inclusive)
    bool                     top_level_only = true;  // WHERE parent_id IS NULL
    int                      limit  = 100;
    int                      offset = 0;
};

// ── RunModuleSummary ─────────────────────────────────────────────────────────
// Row from the run_summaries table.
struct RunModuleSummary {
    std::string module_id;
    std::string test_id;
    int16_t     last_status   = 0;
    int16_t     last_severity = 0;
    std::string last_timestamp;
    int64_t     result_count  = 0;
};

// ── TrendPoint ───────────────────────────────────────────────────────────────
// One data-point for the module trend chart.
struct TrendPoint {
    std::string timestamp;
    double      value    = 0.0;   // extracted numeric value
    int16_t     status   = 0;
    int16_t     severity = 0;
};

// ── CompareEntry ─────────────────────────────────────────────────────────────
// Per-(module_id, test_id) comparison between two runs.
struct CompareEntry {
    std::string module_id;
    std::string test_id;
    std::string classification;          // set by CompareService::classify()
    std::optional<int16_t> run_a_status;
    std::optional<int16_t> run_b_status;
    int16_t     run_a_severity = 0;
    int16_t     run_b_severity = 0;
    std::string run_a_metrics_json;
    std::string run_b_metrics_json;
};

// ── ResultRepository ─────────────────────────────────────────────────────────
class ResultRepository {
public:
    // Insert a single result row.  Recursive subtests are inserted with
    // parent_id = inserted_id in the same transaction.
    Result insert(const Result& r);

    std::optional<Result> find_by_id(const std::string& id);
    std::vector<Result>   list(const ResultFilter& f);

    // Returns result count for a run (for pagination metadata).
    int64_t count_for_run(const std::string& run_id);

    // Returns the most-recent result per (module_id, test_id) for a run,
    // using the run_summaries table for O(1) lookup.
    std::vector<RunModuleSummary> find_last_per_module(const std::string& run_id);

    // Time-series extraction: returns metric values for a (module_id, test_id, field)
    // over the given time range.  Only results where metrics->>field is numeric are returned.
    std::vector<TrendPoint> find_module_trend(const std::string& module_id,
                                               const std::string& test_id,
                                               const std::string& field,
                                               const std::string& from_ts,
                                               const std::string& to_ts);

    // Distribution: returns the numeric values of a metric across the given run IDs.
    // Used for histogram rendering.
    std::vector<double> find_distribution(const std::string& module_id,
                                           const std::string& test_id,
                                           const std::string& field,
                                           const std::vector<std::string>& run_ids);

    // Compare: returns per-(module_id,test_id) last-status/severity for two runs.
    std::vector<CompareEntry> find_for_compare(const std::string& run_a,
                                                const std::string& run_b);

    // Lazy subtest fetch: returns direct children of a result (one level only).
    std::vector<Result> get_subtests(const std::string& parent_id);

    // Upsert run_summaries for a result.  Called after every insert.
    void upsert_run_summary(const std::string& run_id,
                             const std::string& module_id,
                             const std::string& test_id,
                             int16_t status,
                             int16_t severity,
                             const std::string& timestamp);
};

}  // namespace zion
