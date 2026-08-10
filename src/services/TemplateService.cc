#include "services/TemplateService.h"
#include "util/Errors.h"

namespace zion {

Template TemplateService::create(const Template& t, const std::string& actor_user_id) {
    Template copy = t;
    copy.created_by = actor_user_id;
    auto created = repo_.create(copy);
    audit_repo_.log(actor_user_id, "template.created", "template", created.id, "{}");
    return created;
}

Template TemplateService::get(const std::string& id) {
    auto opt = repo_.find_by_id(id);
    if (!opt) throw NotFoundError("template " + id);
    return *opt;
}

std::vector<Template> TemplateService::list(bool include_archived) {
    return repo_.list(include_archived);
}

Template TemplateService::update(const Template& t, const std::string& actor_user_id) {
    repo_.update(t);
    audit_repo_.log(actor_user_id, "template.updated", "template", t.id, "{}");
    return get(t.id);
}

void TemplateService::archive(const std::string& id, const std::string& actor_user_id) {
    repo_.archive(id);
    audit_repo_.log(actor_user_id, "template.archived", "template", id, "{}");
}

void TemplateService::remove(const std::string& id, const std::string& actor_user_id) {
    repo_.remove(id);
    audit_repo_.log(actor_user_id, "template.deleted", "template", id, "{}");
}

std::string TemplateService::instantiate_to_json(const std::string& template_id) {
    auto tpl = get(template_id);
    // Build a JSON object that looks like a TestConfig draft from the template.
    return std::string("{\"name\":\"") + tpl.name + " (copy)\",\"description\":\"" +
           tpl.description + "\",\"mode\":\"" + tpl.mode +
           "\",\"common_config\":" + tpl.common_config_json +
           ",\"modules\":" + tpl.modules_json + "}";
}

}  // namespace zion
