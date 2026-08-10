#pragma once
// repositories/TemplateRepository.h — same shape as TestConfig but for reusable templates.
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct Template {
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

class TemplateRepository {
public:
    Template              create(const Template& t);
    std::optional<Template> find_by_id(const std::string& id);
    std::vector<Template> list(bool include_archived = false);
    void                  update(const Template& t);
    void                  archive(const std::string& id);
    void                  remove(const std::string& id);
};

}  // namespace zion
