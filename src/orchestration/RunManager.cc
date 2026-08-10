#include "orchestration/RunManager.h"
#include "services/RunService.h"
#include <spdlog/spdlog.h>
#include <sstream>

namespace zion::orchestration {

RunManager::RunManager(RunService&          run_svc,
                        LogService&          log_svc,
                        NotificationService& notif_svc)
    : run_svc_(run_svc), log_svc_(log_svc), notif_svc_(notif_svc) {}

void RunManager::on_run_completed(const std::string& run_id,
                                   const std::string& final_status) {
    spdlog::info("RunManager: run_id={} completed status={}", run_id, final_status);
    run_svc_.complete_run(run_id, final_status);
    // Notification for "run.failed" is emitted inside complete_run().
}

void RunManager::on_result(const std::string& run_id, const Result& result) {
    auto saved = run_svc_.ingest_result(result);
    spdlog::debug("RunManager: ingested result run_id={} module_id={} status={}",
                  run_id, saved.module_id, saved.status);

    // ── Notification triggers ────────────────────────────────────────────────
    // test.failure: FAIL/ERROR/TIMEOUT with ERROR or CRITICAL severity.
    if (RunService::is_terminal_failure(saved.status, saved.severity)) {
        std::ostringstream p;
        p << R"({"run_id":")"   << run_id          << "\","
          << R"("module_id":")" << saved.module_id  << "\","
          << R"("test_id":")"   << saved.test_id    << "\","
          << R"("status":")"    << saved.status     << "\","
          << R"("severity":")"  << saved.severity   << "\","
          << R"("result_id":")" << saved.id         << "\"}";
        notif_svc_.emit("test.failure", p.str());
        spdlog::info("RunManager: test.failure emitted run_id={} module_id={}",
                     run_id, saved.module_id);
    }

    // test.warning: FAIL/ERROR with WARNING severity.
    if (RunService::is_warning(saved.status, saved.severity)) {
        std::ostringstream p;
        p << R"({"run_id":")"   << run_id          << "\","
          << R"("module_id":")" << saved.module_id  << "\","
          << R"("test_id":")"   << saved.test_id    << "\","
          << R"("result_id":")" << saved.id         << "\"}";
        notif_svc_.emit("test.warning", p.str());
    }
}

}  // namespace zion::orchestration
