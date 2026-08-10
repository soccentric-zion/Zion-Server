#pragma once
// orchestration/TargetHeartbeat.h — monitors board liveness and updates status.
#include "services/TargetService.h"
#include <atomic>
#include <thread>

namespace zion::orchestration {

class TargetHeartbeat {
public:
    explicit TargetHeartbeat(TargetService& target_svc,
                              std::chrono::seconds interval = std::chrono::seconds{30});
    ~TargetHeartbeat();

    void start();
    void stop();

private:
    void loop();

    TargetService&        target_svc_;
    std::chrono::seconds  interval_;
    std::atomic<bool>     running_{false};
    std::thread           thread_;
};

}  // namespace zion::orchestration
