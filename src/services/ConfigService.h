#pragma once
// services/ConfigService.h
#include "repositories/TestConfigRepository.h"
#include "repositories/AuditRepository.h"
#include <optional>
#include <string>
#include <vector>

namespace zion {

class ConfigService {
public:
    TestConfig create(const TestConfig& c, const std::string& actor_user_id);
    TestConfig get(const std::string& id);
    std::vector<TestConfig> list(bool include_archived = false);
    TestConfig update(const TestConfig& c, const std::string& actor_user_id);
    void archive(const std::string& id, const std::string& actor_user_id);
    void remove(const std::string& id, const std::string& actor_user_id);

private:
    TestConfigRepository repo_;
    AuditRepository      audit_repo_;
};

}  // namespace zion
