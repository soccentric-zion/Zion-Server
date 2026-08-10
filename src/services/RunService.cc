#include "services/RunService.h"
#include "util/Errors.h"
#include "util/ResultConvert.h"

#include <spdlog/spdlog.h>

#include <mutex>
#include <stdexcept>

namespace zion {

Run RunService::start_run(const std::string& config_snapshot_json,
                           const std::string& mode,
                           const std::string& name,
                           const std::string& actor_user_id) {
    Run r;
    r.name                      = name.empty() ? "Run" : name;
    r.test_config_snapshot_json = config_snapshot_json;
    r.mode                      = mode;
    r.status                    = "queued";
    r.created_by                = actor_user_id;
    auto created = run_repo_.create(r);
    audit_repo_.log(actor_user_id, "run.created", "run", created.id, "{}");
    spdlog::info("run: created id={} mode={}", created.id, mode);
    return created;
}

Run RunService::get_run(const std::string& id) {
    auto opt = run_repo_.find_by_id(id);
    if (!opt) throw NotFoundError("run " + id);
    return *opt;
}

std::vector<Run> RunService::list_runs(const RunFilter& f) {
    return run_repo_.list(f);
}

void RunService::mark_running(const std::string& run_id) {
    run_repo_.set_started(run_id);
    audit_repo_.log("", "run.started", "run", run_id, "{}");
    spdlog::info("run: started id={}", run_id);
}

Result RunService::ingest_result(const Result& r) {
    auto saved = result_repo_.insert(r);
    broadcast_result(r.run_id, saved);
    return saved;
}

Result RunService::ingest_test_result(const hal::common::TestResult& tr,
                                       const std::string& run_id,
                                       const std::string& module_version,
                                       const std::string& mode) {
    // Guard: reject UNSPECIFIED status — the Client should never produce this.
    if (tr.output().status() == hal::common::TEST_STATUS_UNSPECIFIED) {
        throw ValidationError("TestResult.output.status must not be UNSPECIFIED");
    }

    Result r = util::proto_to_result(tr, run_id, module_version, mode);
    auto saved = result_repo_.insert(r);

    spdlog::debug("run: ingested result run_id={} module_id={} test_id={} status={}",
                  run_id, saved.module_id, saved.test_id, saved.status);

    broadcast_result(run_id, saved);
    return saved;
}

// static
bool RunService::is_terminal_failure(int16_t status, int16_t severity) {
    // FAIL=2, ERROR=3, TIMEOUT=4 with ERROR=3 or CRITICAL=4 severity.
    bool bad_status   = (status == 2 || status == 3 || status == 4);
    bool bad_severity = (severity == 3 || severity == 4);
    return bad_status && bad_severity;
}

// static
bool RunService::is_warning(int16_t status, int16_t severity) {
    // FAIL=2 or ERROR=3 with WARNING=2 severity.
    bool bad_status = (status == 2 || status == 3);
    return bad_status && (severity == 2);
}

// static
std::string RunService::module_badge(const RunModuleSummary& s,
                                      bool module_enabled_in_run) {
    if (!module_enabled_in_run || s.result_count == 0)
        return "NEVER";

    switch (s.last_status) {
    case 1: return "PASS";       // TEST_STATUS_PASS
    case 2: return "FAIL";       // TEST_STATUS_FAIL
    case 3: return "FAIL";       // TEST_STATUS_ERROR → treat as FAIL badge
    case 4: return "FAIL";       // TEST_STATUS_TIMEOUT → treat as FAIL badge
    case 5: return "ABORTED";    // TEST_STATUS_ABORTED
    case 7: return "NEVER";      // TEST_STATUS_SKIPPED → counts as no result
    default: return "NEVER";
    }
}

void RunService::complete_run(const std::string& run_id, const std::string& status,
                               const std::string& error) {
    run_repo_.set_ended(run_id, status, error);
    audit_repo_.log("", "run." + status, "run", run_id,
                    error.empty() ? "{}" : R"({"error":")" + error + "\"}");
    spdlog::info("run: {} id={}", status, run_id);

    if (status == "completed" || status == "failed") {
        std::string kind = (status == "failed") ? "run.failed" : "run.completed";
        notif_repo_.create(kind,
            R"({"run_id":")" + run_id + R"(","status":")" + status + "\"}");
    } else if (status == "cancelled") {
        notif_repo_.create("run.aborted",
            R"({"run_id":")" + run_id + "\"}");
        audit_repo_.log("", "run.aborted_with_partial_results", "run", run_id, "{}");
    }
}

void RunService::cancel_run(const std::string& run_id, const std::string& actor_user_id) {
    run_repo_.update_status(run_id, "cancelled");
    audit_repo_.log(actor_user_id, "run.cancelled", "run", run_id, "{}");
}

void RunService::pause_run(const std::string& run_id, const std::string& actor_user_id) {
    run_repo_.update_status(run_id, "paused");
    audit_repo_.log(actor_user_id, "run.paused", "run", run_id, "{}");
}

void RunService::resume_run(const std::string& run_id, const std::string& actor_user_id) {
    run_repo_.update_status(run_id, "running");
    audit_repo_.log(actor_user_id, "run.resumed", "run", run_id, "{}");
}

uint64_t RunService::subscribe_results(const std::string& run_id,
                                        ResultSubscriber cb) {
    std::lock_guard<std::mutex> lock{subs_mutex_};
    uint64_t id = next_sub_id_++;
    subs_[id] = std::move(cb);
    run_subs_[run_id].push_back(id);
    return id;
}

void RunService::unsubscribe_results(uint64_t sub_id) {
    std::lock_guard<std::mutex> lock{subs_mutex_};
    subs_.erase(sub_id);
    for (auto& [rid, ids] : run_subs_) {
        ids.erase(std::remove(ids.begin(), ids.end(), sub_id), ids.end());
    }
}

void RunService::broadcast_result(const std::string& run_id, const Result& r) {
    std::lock_guard<std::mutex> lock{subs_mutex_};
    auto it = run_subs_.find(run_id);
    if (it == run_subs_.end()) return;
    for (uint64_t sub_id : it->second) {
        auto sit = subs_.find(sub_id);
        if (sit != subs_.end()) {
            try { sit->second(r); } catch (...) {}
        }
    }
}

std::vector<Result> RunService::get_results(const ResultFilter& f) {
    return result_repo_.list(f);
}

std::optional<Result> RunService::get_result(const std::string& id) {
    return result_repo_.find_by_id(id);
}

std::vector<RunModuleSummary> RunService::get_module_summaries(const std::string& run_id) {
    return result_repo_.find_last_per_module(run_id);
}

std::vector<TrendPoint> RunService::get_module_trend(const std::string& module_id,
                                                      const std::string& test_id,
                                                      const std::string& field,
                                                      const std::string& from_ts,
                                                      const std::string& to_ts) {
    return result_repo_.find_module_trend(module_id, test_id, field, from_ts, to_ts);
}

std::vector<double> RunService::get_distribution(const std::string& module_id,
                                                   const std::string& test_id,
                                                   const std::string& field,
                                                   const std::vector<std::string>& run_ids) {
    return result_repo_.find_distribution(module_id, test_id, field, run_ids);
}

std::vector<CompareEntry> RunService::get_compare(const std::string& run_a,
                                                   const std::string& run_b) {
    return result_repo_.find_for_compare(run_a, run_b);
}

std::vector<Result> RunService::get_subtests(const std::string& parent_id) {
    return result_repo_.get_subtests(parent_id);
}

Run RunService::rerun(const std::string& run_id, const std::string& actor_user_id) {
    auto orig = get_run(run_id);
    return start_run(orig.test_config_snapshot_json, orig.mode,
                     "Rerun of " + orig.name, actor_user_id);
}

void RunService::set_baseline(const std::string& run_id,
                               const std::string& actor_user_id) {
    auto run = get_run(run_id);
    run_repo_.set_baseline(run_id, run.mode);
    audit_repo_.log(actor_user_id, "run.baseline_set", "run", run_id, "{}");
}

void RunService::delete_run(const std::string& run_id,
                             const std::string& actor_user_id) {
    run_repo_.remove(run_id);
    audit_repo_.log(actor_user_id, "run.deleted", "run", run_id, "{}");
}

int RunService::reconcile_stale_runs() {
    int count = 0;
    for (const char* stale_status : {"running", "queued"}) {
        RunFilter f;
        f.status = stale_status;
        auto stale = run_repo_.list(f);
        for (const auto& r : stale) {
            run_repo_.update_status(r.id, "failed",
                                    "Server restarted — run was interrupted");
            ++count;
        }
    }
    return count;
}

}  // namespace zion
