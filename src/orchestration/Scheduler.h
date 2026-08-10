#pragma once
// orchestration/Scheduler.h — fires scheduled runs on cron intervals.
#include "services/ScheduleService.h"
#include "services/RunService.h"

#include <atomic>
#include <thread>

namespace zion::orchestration {

class Scheduler {
public:
    Scheduler(ScheduleService& schedule_svc, RunService& run_svc);
    ~Scheduler();

    void start();
    void stop();

private:
    void loop();

    ScheduleService& schedule_svc_;
    RunService&      run_svc_;
    std::atomic<bool> running_{false};
    std::thread       thread_;
};

}  // namespace zion::orchestration
