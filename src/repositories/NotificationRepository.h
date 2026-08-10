#pragma once
// repositories/NotificationRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct Notification {
    std::string id;
    std::string kind;
    std::string payload_json;
    std::string read_at;    // empty if unread
    std::string created_at;
};

struct NotificationFilter {
    bool unread_only = false;
    int  limit  = 50;
    int  offset = 0;
};

class NotificationRepository {
public:
    Notification              create(const std::string& kind,
                                     const std::string& payload_json);
    std::vector<Notification> list(const NotificationFilter& f);
    void                      mark_read(const std::string& id);
    void                      mark_all_read();
    int64_t                   unread_count();
};

}  // namespace zion
