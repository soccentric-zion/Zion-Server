#pragma once
// services/RunService.h — run lifecycle management.
#include "repositories/RunRepository.h"
#include "repositories/ResultRepository.h"
#include "repositories/AuditRepository.h"
#include "repositories/NotificationRepository.h"

#include "common.pb.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace zion {

// Subscriber callback for live result streaming to WebSocket/SSE clients.
using ResultSubscriber = std::function<void(const Result&)>;

class RunService {
public:
    // Create and start (enqueue) a new run.
    Run start_run(const std::string& config_snapshot_json,
                  const std::string& mode,
                  const std::string& name,
                  const std::string& actor_user_id);

    Run get_run(const std::string& id);
    std::vector<Run> list_runs(const RunFilter& f);

    // Transition the run to 'running'.
    void mark_running(const std::string& run_id);

    // Persist a result that has already been converted to the DB struct.
    // This path is kept for backward compatibility with legacy call sites.
    Result ingest_result(const Result& r);

    // Primary ingest path: persist a TestResult proto received from the Client.
    // Converts the proto to a Result struct, inserts (with subtests), returns saved row.
    // Validates status != UNSPECIFIED; throws ValidationError if unspecified.
    Result ingest_test_result(const hal::common::TestResult& tr,
                              const std::string& run_id,
                              const std::string& module_version,
                              const std::string& mode);

    // Returns true if the (status, severity) pair qualifies as a terminal failure
    // that should trigger a "test.failure" notification.
    // Rules: FAIL/ERROR/TIMEOUT with severity ERROR or CRITICAL qualifies.
    //        SKIPPED, ABORTED do not qualify.
    static bool is_terminal_failure(int16_t status, int16_t severity);

    // Returns true if (status, severity) qualifies as a warning notification.
    // Rules: FAIL/ERROR with severity WARNING.
    static bool is_warning(int16_t status, int16_t severity);

    // Derive per-module badge from run summaries.
    // Returns "PASS", "FAIL", "ABORTED", or "NEVER" per the spec rules.
    static std::string module_badge(const RunModuleSummary& s, bool module_enabled_in_run);

    // Finalize the run.
    void complete_run(const std::string& run_id, const std::string& status,
                      const std::string& error = "");

    void cancel_run(const std::string& run_id, const std::string& actor_user_id);
    void pause_run(const std::string& run_id, const std::string& actor_user_id);
    void resume_run(const std::string& run_id, const std::string& actor_user_id);

    // Subscribe to live results for a run (REST/WebSocket live tail).
    uint64_t subscribe_results(const std::string& run_id, ResultSubscriber cb);
    void     unsubscribe_results(uint64_t sub_id);

    // Notify all subscribers of a new result.
    void broadcast_result(const std::string& run_id, const Result& r);

    // Query results.
    std::vector<Result> get_results(const ResultFilter& f);
    std::optional<Result> get_result(const std::string& id);

    // Per-module summary for a run.
    std::vector<RunModuleSummary> get_module_summaries(const std::string& run_id);

    // Trend / analytics delegates.
    std::vector<TrendPoint> get_module_trend(const std::string& module_id,
                                              const std::string& test_id,
                                              const std::string& field,
                                              const std::string& from_ts,
                                              const std::string& to_ts);

    std::vector<double> get_distribution(const std::string& module_id,
                                          const std::string& test_id,
                                          const std::string& field,
                                          const std::vector<std::string>& run_ids);

    std::vector<CompareEntry> get_compare(const std::string& run_a,
                                           const std::string& run_b);

    std::vector<Result> get_subtests(const std::string& parent_id);

    // Rerun a finished run with the same config snapshot.
    Run rerun(const std::string& run_id, const std::string& actor_user_id);

    // Set baseline.
    void set_baseline(const std::string& run_id, const std::string& actor_user_id);

    void delete_run(const std::string& run_id, const std::string& actor_user_id);

    // On-startup reconciliation: mark any runs still in 'running' or 'queued'
    // state as 'failed' (the server crashed before completing them).
    // Returns the number of runs reconciled.
    int reconcile_stale_runs();

private:
    RunRepository        run_repo_;
    ResultRepository     result_repo_;
    AuditRepository      audit_repo_;
    NotificationRepository notif_repo_;

    std::mutex                                         subs_mutex_;
    std::unordered_map<uint64_t, ResultSubscriber>     subs_;
    std::unordered_map<std::string, std::vector<uint64_t>> run_subs_;
    std::atomic<uint64_t>                              next_sub_id_{1};
};

}  // namespace zion
