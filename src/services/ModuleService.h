#pragma once
// services/ModuleService.h — expose the module registry over REST.
#include "registry/ModuleRegistry.h"
#include <optional>
#include <string>
#include <vector>

namespace zion {

class ModuleService {
public:
    explicit ModuleService(const ModuleRegistry& registry);

    const std::vector<ModuleInfo>& list_all() const;
    std::optional<ModuleInfo> get(const std::string& module_id) const;
    bool validate(const std::string& module_id) const;

private:
    const ModuleRegistry& registry_;
};

}  // namespace zion
