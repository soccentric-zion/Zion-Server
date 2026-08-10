#include "services/ConfigService.h"
#include "util/Errors.h"

namespace zion {

TestConfig ConfigService::create(const TestConfig& c, const std::string& actor_user_id) {
    TestConfig copy = c;
    copy.created_by = actor_user_id;
    auto created = repo_.create(copy);
    audit_repo_.log(actor_user_id, "config.created", "test_config", created.id, "{}");
    return created;
}

TestConfig ConfigService::get(const std::string& id) {
    auto opt = repo_.find_by_id(id);
    if (!opt) throw NotFoundError("config " + id);
    return *opt;
}

std::vector<TestConfig> ConfigService::list(bool include_archived) {
    return repo_.list(include_archived);
}

TestConfig ConfigService::update(const TestConfig& c, const std::string& actor_user_id) {
    repo_.update(c);
    audit_repo_.log(actor_user_id, "config.updated", "test_config", c.id, "{}");
    return get(c.id);
}

void ConfigService::archive(const std::string& id, const std::string& actor_user_id) {
    repo_.archive(id);
    audit_repo_.log(actor_user_id, "config.archived", "test_config", id, "{}");
}

void ConfigService::remove(const std::string& id, const std::string& actor_user_id) {
    repo_.remove(id);
    audit_repo_.log(actor_user_id, "config.deleted", "test_config", id, "{}");
}

}  // namespace zion
