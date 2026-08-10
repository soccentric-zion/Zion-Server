#pragma once
// services/TemplateService.h
#include "repositories/TemplateRepository.h"
#include "repositories/AuditRepository.h"
#include <optional>
#include <string>
#include <vector>

namespace zion {

class TemplateService {
public:
    Template create(const Template& t, const std::string& actor_user_id);
    Template get(const std::string& id);
    std::vector<Template> list(bool include_archived = false);
    Template update(const Template& t, const std::string& actor_user_id);
    void archive(const std::string& id, const std::string& actor_user_id);
    void remove(const std::string& id, const std::string& actor_user_id);
    // Instantiate a template into a draft TestConfig (returned as JSON blob)
    std::string instantiate_to_json(const std::string& template_id);

private:
    TemplateRepository repo_;
    AuditRepository    audit_repo_;
};

}  // namespace zion
