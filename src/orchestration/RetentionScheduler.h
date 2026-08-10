#pragma once
// orchestration/RetentionScheduler.h — background thread that enforces data
// retention policy by deleting old (non-baseline) runs.

#include "services/RunService.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace zion {

class RetentionScheduler {
public:
    // run_svc    – used to delete old runs
    // max_age_days – runs older than this (ended_at) are deleted; 0 = disabled
    // interval   – how often to run the sweep (default: daily)
    explicit RetentionScheduler(RunService& run_svc,
                                 int max_age_days = 90,
                                 std::chrono::seconds interval = std::chrono::hours(24));

    ~RetentionScheduler();

    void start();
    void stop();

    // Expose/update the max age without restarting.
    int  max_age_days() const noexcept { return max_age_days_; }
    void set_max_age_days(int d) noexcept { max_age_days_ = d; }

private:
    void run_loop();
    void sweep();

    RunService&            run_svc_;
    int                    max_age_days_;
    std::chrono::seconds   interval_;
    std::atomic<bool>      running_{false};
    std::thread            thread_;
};

}  // namespace zion
