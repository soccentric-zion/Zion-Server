#pragma once
// orchestration/RunManager.h — coordinates run lifecycle across gRPC and REST.
#include "services/RunService.h"
#include "services/LogService.h"
#include "services/NotificationService.h"

namespace zion::orchestration {

class RunManager {
public:
    RunManager(RunService&          run_svc,
               LogService&          log_svc,
               NotificationService& notif_svc);

    // Called by gRPC handler when a run completes.
    void on_run_completed(const std::string& run_id, const std::string& final_status);

    // Called by gRPC handler when a result envelope arrives.
    void on_result(const std::string& run_id, const Result& result);

private:
    RunService&          run_svc_;
    LogService&          log_svc_;
    NotificationService& notif_svc_;
};

}  // namespace zion::orchestration
