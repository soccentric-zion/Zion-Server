#include "orchestration/OtaOrchestrator.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace zion::orchestration {

OtaOrchestrator::OtaOrchestrator(OtaService& ota_svc) : ota_svc_(ota_svc) {}
OtaOrchestrator::~OtaOrchestrator() { stop(); }

void OtaOrchestrator::start() {
    running_ = true;
    thread_ = std::thread([this] { loop(); });
    spdlog::info("OtaOrchestrator started");
}

void OtaOrchestrator::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void OtaOrchestrator::loop() {
    while (running_) {
        try {
            auto pending = ota_svc_.list();
            for (const auto& u : pending) {
                if (u.status == "pending") {
                    process(u);
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("OtaOrchestrator error: {}", e.what());
        }
        std::this_thread::sleep_for(std::chrono::seconds{5});
    }
}

void OtaOrchestrator::process(const OtaUpgrade& u) {
    spdlog::info("OtaOrchestrator: processing upgrade id={}", u.id);
    // Simulate upgrade progression.
    // In production, this would invoke the board's OTA update mechanism.
    ota_svc_.update_progress(u.id, 50, "in_progress");
    // Assume success (real impl would check board acknowledgement).
    ota_svc_.finalize(u.id, "completed");
    spdlog::info("OtaOrchestrator: upgrade id={} completed", u.id);
}

}  // namespace zion::orchestration
