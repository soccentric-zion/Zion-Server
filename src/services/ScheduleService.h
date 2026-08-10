#pragma once
// services/ScheduleService.h
#include "repositories/ScheduleRepository.h"
#include "repositories/AuditRepository.h"
#include <optional>
#include <string>
#include <vector>

namespace zion {

class ScheduleService {
public:
    Schedule create(const Schedule& s, const std::string& actor_user_id);
    Schedule get(const std::string& id);
    std::vector<Schedule> list_enabled();
    Schedule update(const Schedule& s, const std::string& actor_user_id);
    void set_enabled(const std::string& id, bool enabled, const std::string& actor_user_id);
    void remove(const std::string& id, const std::string& actor_user_id);

    // Returns schedules that are due to fire (next_run_at <= now).
    std::vector<Schedule> get_due_schedules();

    // Record that a schedule was triggered; advance next_run_at.
    void record_fired(const std::string& id, const std::string& next_run_at);

private:
    ScheduleRepository repo_;
    AuditRepository    audit_repo_;

    // Compute the next run timestamp for a cron expression.
    static std::string compute_next_run(const std::string& cron_expr);
};

}  // namespace zion
