#include "orchestration/RetentionScheduler.h"
#include "repositories/RunRepository.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace zion {

RetentionScheduler::RetentionScheduler(RunService& run_svc,
                                         int max_age_days,
                                         std::chrono::seconds interval)
    : run_svc_(run_svc),
      max_age_days_(max_age_days),
      interval_(interval) {}

RetentionScheduler::~RetentionScheduler() {
    stop();
}

void RetentionScheduler::start() {
    if (running_.exchange(true)) return;  // already running
    thread_ = std::thread(&RetentionScheduler::run_loop, this);
    spdlog::info("RetentionScheduler started (max_age_days={})", max_age_days_);
}

void RetentionScheduler::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

void RetentionScheduler::run_loop() {
    // Do an immediate sweep on startup, then repeat at interval_.
    while (running_.load()) {
        sweep();
        // Sleep in short bursts so we can respond to stop() promptly.
        auto deadline = std::chrono::steady_clock::now() + interval_;
        while (running_.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void RetentionScheduler::sweep() {
    int days = max_age_days_;
    if (days <= 0) return;  // retention disabled

    // Build cutoff timestamp string: now - max_age_days_ in ISO-8601 UTC.
    auto now   = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::hours(24 * days);
    std::time_t cutoff_t = std::chrono::system_clock::to_time_t(cutoff);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&cutoff_t), "%Y-%m-%dT%H:%M:%SZ");
    std::string cutoff_str = oss.str();

    // List all finished runs ended before the cutoff (non-baseline only).
    RunFilter f;
    f.to_ts  = cutoff_str;
    f.limit  = 200;   // process in batches
    f.offset = 0;

    int deleted = 0;
    while (true) {
        auto runs = run_svc_.list_runs(f);
        if (runs.empty()) break;
        for (const auto& r : runs) {
            if (r.baseline_flag) continue;  // never delete baselines
            const std::string& s = r.status;
            if (s != "completed" && s != "failed" && s != "cancelled") continue;
            try {
                run_svc_.delete_run(r.id, "system");
                ++deleted;
            } catch (const std::exception& ex) {
                spdlog::warn("RetentionScheduler: failed to delete run {}: {}", r.id, ex.what());
            }
        }
        if (static_cast<int>(runs.size()) < f.limit) break;
        f.offset += f.limit;
    }

    if (deleted > 0) {
        spdlog::info("RetentionScheduler: deleted {} run(s) older than {} days", deleted, days);
    }
}

}  // namespace zion
