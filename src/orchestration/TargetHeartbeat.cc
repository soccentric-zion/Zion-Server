#include "orchestration/TargetHeartbeat.h"
#include "util/Errors.h"
#include <spdlog/spdlog.h>

namespace zion::orchestration {

TargetHeartbeat::TargetHeartbeat(TargetService& target_svc,
                                   std::chrono::seconds interval)
    : target_svc_(target_svc), interval_(interval) {}

TargetHeartbeat::~TargetHeartbeat() { stop(); }

void TargetHeartbeat::start() {
    running_ = true;
    thread_ = std::thread([this] { loop(); });
}

void TargetHeartbeat::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void TargetHeartbeat::loop() {
    while (running_) {
        std::this_thread::sleep_for(interval_);
        try {
            auto board = target_svc_.get_board();
            if (board) {
                target_svc_.heartbeat(board->id, "online");
            }
        } catch (const NotFoundError&) {
            // No board registered yet — silently ignore.
        } catch (const std::exception& e) {
            spdlog::warn("TargetHeartbeat error: {}", e.what());
        }
    }
}

}  // namespace zion::orchestration
