#pragma once
// repositories/LogRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct LogEntry {
    std::string id;
    std::string source;
    std::string severity;
    std::string timestamp;
    std::string message;
    std::string metadata_json;
    std::string run_id;  // empty if not correlated
};

struct LogFilter {
    std::string source;
    std::string severity;
    std::string from_ts;
    std::string to_ts;
    std::string run_id;
    std::string query;  // full-text search
    int limit  = 100;
    int offset = 0;
};

struct LogBookmark {
    std::string id;
    std::string user_id;
    std::string log_id;
    std::string note;
    std::string created_at;
};

struct SavedLogView {
    std::string id;
    std::string user_id;
    std::string name;
    std::string filters_json;
    std::string created_at;
};

class LogRepository {
public:
    LogEntry             insert(const LogEntry& e);
    std::vector<LogEntry> list(const LogFilter& f);
    int64_t              count(const LogFilter& f);

    LogBookmark              create_bookmark(const std::string& user_id,
                                             const std::string& log_id,
                                             const std::string& note);
    std::vector<LogBookmark> list_bookmarks(const std::string& user_id);
    void                     remove_bookmark(const std::string& id);

    SavedLogView              create_view(const std::string& user_id,
                                          const std::string& name,
                                          const std::string& filters_json);
    std::vector<SavedLogView> list_views(const std::string& user_id);
};

}  // namespace zion
