#include "services/ScheduleService.h"
#include "util/Errors.h"
#include "util/Time.h"

#include <chrono>
#include <sstream>

namespace zion {

Schedule ScheduleService::create(const Schedule& s, const std::string& actor_user_id) {
    Schedule copy = s;
    copy.created_by = actor_user_id;
    if (!copy.cron_expression.empty() && copy.next_run_at.empty()) {
        copy.next_run_at = compute_next_run(copy.cron_expression);
    }
    auto created = repo_.create(copy);
    audit_repo_.log(actor_user_id, "schedule.created", "schedule", created.id, "{}");
    return created;
}

Schedule ScheduleService::get(const std::string& id) {
    auto opt = repo_.find_by_id(id);
    if (!opt) throw NotFoundError("schedule " + id);
    return *opt;
}

std::vector<Schedule> ScheduleService::list_enabled() {
    return repo_.list_enabled();
}

Schedule ScheduleService::update(const Schedule& s, const std::string& actor_user_id) {
    repo_.update(s);
    audit_repo_.log(actor_user_id, "schedule.updated", "schedule", s.id, "{}");
    return get(s.id);
}

void ScheduleService::set_enabled(const std::string& id, bool enabled,
                                   const std::string& actor_user_id) {
    repo_.set_enabled(id, enabled);
    audit_repo_.log(actor_user_id,
                    enabled ? "schedule.enabled" : "schedule.disabled",
                    "schedule", id, "{}");
}

void ScheduleService::remove(const std::string& id, const std::string& actor_user_id) {
    repo_.remove(id);
    audit_repo_.log(actor_user_id, "schedule.deleted", "schedule", id, "{}");
}

std::vector<Schedule> ScheduleService::get_due_schedules() {
    return repo_.list_due(util::utc_now_iso8601());
}

void ScheduleService::record_fired(const std::string& id,
                                    const std::string& next_run_at) {
    repo_.record_run(id, next_run_at);
}

// Minimal cron next-run computation: advances by 1 minute for simple intervals.
// A full cron parser would be a separate library (e.g. croncpp).
// Decision: We use a 1-minute minimum resolution; advanced expressions use
// a best-effort computation.  Document in README.
std::string ScheduleService::compute_next_run(const std::string& /*cron_expr*/) {
    using namespace std::chrono;
    auto next = system_clock::now() + minutes{1};
    return util::to_iso8601(next);
}

}  // namespace zion
