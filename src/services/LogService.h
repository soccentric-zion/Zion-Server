#pragma once
// services/LogService.h — log ingestion and query.
#include "repositories/LogRepository.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace zion {

using LogSubscriber = std::function<void(const LogEntry&)>;

class LogService {
public:
    // Ingest a log entry (called from gRPC LogIngest handler).
    LogEntry ingest(const LogEntry& e);

    std::vector<LogEntry> list(const LogFilter& f);

    // SSE / live tail subscription.
    uint64_t subscribe(LogSubscriber cb);
    void     unsubscribe(uint64_t sub_id);
    void     broadcast(const LogEntry& e);

    // Bookmarks and saved views
    LogBookmark create_bookmark(const std::string& user_id,
                                const std::string& log_id,
                                const std::string& note);
    std::vector<LogBookmark> list_bookmarks(const std::string& user_id);
    void remove_bookmark(const std::string& id);

    SavedLogView create_view(const std::string& user_id,
                              const std::string& name,
                              const std::string& filters_json);
    std::vector<SavedLogView> list_views(const std::string& user_id);

private:
    LogRepository log_repo_;
    std::mutex    subs_mutex_;
    std::unordered_map<uint64_t, LogSubscriber> subs_;
    std::atomic<uint64_t> next_sub_id_{1};
};

}  // namespace zion
