#pragma once
// orchestration/OtaOrchestrator.h — polls for pending OTA upgrades and drives them.
#include "services/OtaService.h"
#include <atomic>
#include <thread>

namespace zion::orchestration {

class OtaOrchestrator {
public:
    explicit OtaOrchestrator(OtaService& ota_svc);
    ~OtaOrchestrator();

    void start();
    void stop();

private:
    void loop();
    void process(const OtaUpgrade& u);

    OtaService&       ota_svc_;
    std::atomic<bool> running_{false};
    std::thread       thread_;
};

}  // namespace zion::orchestration
