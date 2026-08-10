#include "services/LogService.h"

namespace zion {

LogEntry LogService::ingest(const LogEntry& e) {
    auto saved = log_repo_.insert(e);
    broadcast(saved);
    return saved;
}

std::vector<LogEntry> LogService::list(const LogFilter& f) {
    return log_repo_.list(f);
}

uint64_t LogService::subscribe(LogSubscriber cb) {
    std::lock_guard<std::mutex> lock{subs_mutex_};
    uint64_t id = next_sub_id_++;
    subs_[id] = std::move(cb);
    return id;
}

void LogService::unsubscribe(uint64_t sub_id) {
    std::lock_guard<std::mutex> lock{subs_mutex_};
    subs_.erase(sub_id);
}

void LogService::broadcast(const LogEntry& e) {
    std::lock_guard<std::mutex> lock{subs_mutex_};
    for (auto& [id, cb] : subs_) {
        try { cb(e); } catch (...) {}
    }
}

LogBookmark LogService::create_bookmark(const std::string& user_id,
                                         const std::string& log_id,
                                         const std::string& note) {
    return log_repo_.create_bookmark(user_id, log_id, note);
}

std::vector<LogBookmark> LogService::list_bookmarks(const std::string& user_id) {
    return log_repo_.list_bookmarks(user_id);
}

void LogService::remove_bookmark(const std::string& id) {
    log_repo_.remove_bookmark(id);
}

SavedLogView LogService::create_view(const std::string& user_id,
                                      const std::string& name,
                                      const std::string& filters_json) {
    return log_repo_.create_view(user_id, name, filters_json);
}

std::vector<SavedLogView> LogService::list_views(const std::string& user_id) {
    return log_repo_.list_views(user_id);
}

}  // namespace zion
