#pragma once
// ModuleRegistry — loads protos/registry/modules.yaml at startup.
// Validates all incoming module_id values against the loaded manifest.
// Thread-safe for concurrent reads after init.

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace zion {

struct ModuleInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string version;
    std::string category;        // e.g. "compute", "sensors", "connectivity"
    // proto type_url for the module's Result message
    std::string result_type_url;
    // proto type_url for the module's Config message
    std::string config_type_url;
    // Supported execution modes
    std::vector<std::string> modes;
};

class ModuleRegistry {
public:
    ModuleRegistry() = default;
    ~ModuleRegistry() = default;

    // Load modules.yaml from path.  Throws std::runtime_error on failure.
    void load(const std::string& path);

    // Returns true if the module_id is present in the registry.
    [[nodiscard]] bool has(std::string_view module_id) const;

    // Returns module info, or std::nullopt if not found.
    [[nodiscard]] std::optional<ModuleInfo> find(std::string_view module_id) const;

    // Returns all modules (read-only).
    [[nodiscard]] const std::vector<ModuleInfo>& all() const { return modules_; }

    [[nodiscard]] std::size_t size() const { return modules_.size(); }

private:
    std::unordered_map<std::string, std::size_t> index_;  // id → index in modules_
    std::vector<ModuleInfo>                       modules_;
};

}  // namespace zion
