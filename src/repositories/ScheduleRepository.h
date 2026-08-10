#pragma once
// repositories/ScheduleRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct Schedule {
    std::string id;
    std::string template_id;
    std::string cron_expression;  // empty if one-time
    std::string run_at;           // empty if recurring
    bool        enabled = true;
    std::string last_run_at;
    std::string next_run_at;
    std::string created_at;
    std::string created_by;
};

class ScheduleRepository {
public:
    Schedule              create(const Schedule& s);
    std::optional<Schedule> find_by_id(const std::string& id);
    std::vector<Schedule> list_enabled();
    std::vector<Schedule> list_due(const std::string& now_ts);
    void                  update(const Schedule& s);
    void                  set_enabled(const std::string& id, bool enabled);
    void                  record_run(const std::string& id,
                                     const std::string& next_run_at);
    void                  remove(const std::string& id);
};

}  // namespace zion
