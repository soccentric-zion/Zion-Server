#include "services/ModuleService.h"
#include "util/Errors.h"

namespace zion {

ModuleService::ModuleService(const ModuleRegistry& registry) : registry_(registry) {}

const std::vector<ModuleInfo>& ModuleService::list_all() const {
    return registry_.all();
}

std::optional<ModuleInfo> ModuleService::get(const std::string& module_id) const {
    return registry_.find(module_id);
}

bool ModuleService::validate(const std::string& module_id) const {
    return registry_.has(module_id);
}

}  // namespace zion
