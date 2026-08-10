#pragma once
// services/NotificationService.h — in-app notifications + optional email.
#include "config/Settings.h"
#include "repositories/NotificationRepository.h"
#include <string>
#include <vector>

namespace zion {

class NotificationService {
public:
    explicit NotificationService(const SmtpSettings& smtp);

    void emit(const std::string& kind, const std::string& payload_json);

    std::vector<Notification> list(const NotificationFilter& f);
    void mark_read(const std::string& id);
    void mark_all_read();
    int64_t unread_count();

private:
    SmtpSettings             smtp_;
    NotificationRepository   repo_;

    void send_email(const std::string& subject, const std::string& body);
};

}  // namespace zion
