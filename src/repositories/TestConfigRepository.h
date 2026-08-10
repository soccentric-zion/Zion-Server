#pragma once
// repositories/TestConfigRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct TestConfig {
    std::string id;
    std::string name;
    std::string description;
    std::string mode;
    std::string common_config_json;
    std::string modules_json;
    std::string created_at;
    std::string created_by;
    bool        is_archived = false;
};

class TestConfigRepository {
public:
    TestConfig              create(const TestConfig& c);
    std::optional<TestConfig> find_by_id(const std::string& id);
    std::vector<TestConfig> list(bool include_archived = false);
    void                    update(const TestConfig& c);
    void                    archive(const std::string& id);
    void                    remove(const std::string& id);
};

}  // namespace zion
