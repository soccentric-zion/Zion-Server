#include "services/NotificationService.h"
#include <spdlog/spdlog.h>

namespace zion {

NotificationService::NotificationService(const SmtpSettings& smtp) : smtp_(smtp) {}

void NotificationService::emit(const std::string& kind,
                                const std::string& payload_json) {
    repo_.create(kind, payload_json);

    // Email delivery if configured and kind matches a notification event.
    if (smtp_.host.empty()) return;

    bool send = (kind == "run.failed" || kind == "test.failure" ||
                 kind == "ota.failed");
    if (send) {
        send_email("Zion notification: " + kind,
                   "Event: " + kind + "\nPayload: " + payload_json);
    }
}

std::vector<Notification> NotificationService::list(const NotificationFilter& f) {
    return repo_.list(f);
}

void NotificationService::mark_read(const std::string& id) {
    repo_.mark_read(id);
}

void NotificationService::mark_all_read() {
    repo_.mark_all_read();
}

int64_t NotificationService::unread_count() {
    return repo_.unread_count();
}

void NotificationService::send_email(const std::string& subject,
                                      const std::string& body) {
    // Email via SMTP is handled by the EmailNotifier component which calls
    // a system-level SMTP library.  Here we just log that an email would be sent
    // (the actual implementation is in notifications/EmailNotifier.cc).
    spdlog::info("notification: would send email subject='{}' smtp={}:{}",
                 subject, smtp_.host, smtp_.port);
    (void)body;
}

}  // namespace zion
