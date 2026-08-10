#pragma once
// repositories/RunRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct Run {
    std::string id;
    std::string name;
    std::string description;
    std::string test_config_snapshot_json;
    std::string mode;
    std::string status;
    std::string started_at;
    std::string ended_at;
    std::string error_message;
    std::string tags_json;
    std::string notes;
    bool        baseline_flag = false;
    std::string attached_files_json;
    std::string created_at;
    std::string created_by;
};

struct RunFilter {
    std::string status;
    std::string mode;
    std::string tag;
    std::string from_ts;
    std::string to_ts;
    std::string search;
    int limit  = 50;
    int offset = 0;
};

class RunRepository {
public:
    Run create(const Run& r);
    std::optional<Run> find_by_id(const std::string& id);
    std::vector<Run>   list(const RunFilter& f);
    void update_status(const std::string& id, const std::string& status,
                       const std::string& error_message = "");
    void set_started(const std::string& id);
    void set_ended(const std::string& id, const std::string& status,
                   const std::string& error_message = "");
    void update_notes(const std::string& id, const std::string& notes);
    void update_tags(const std::string& id, const std::string& tags_json);
    void set_baseline(const std::string& id, const std::string& mode);
    void remove(const std::string& id);
};

}  // namespace zion
