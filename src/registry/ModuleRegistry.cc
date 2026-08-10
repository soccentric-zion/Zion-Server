#include "registry/ModuleRegistry.h"

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

namespace zion {

void ModuleRegistry::load(const std::string& path) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("ModuleRegistry: failed to load '" + path + "': " + e.what());
    }

    if (!root["modules"] || !root["modules"].IsSequence()) {
        throw std::runtime_error(
            "ModuleRegistry: expected 'modules' sequence in " + path);
    }

    modules_.clear();
    index_.clear();

    for (const auto& node : root["modules"]) {
        ModuleInfo m;
        m.id          = node["id"]          ? node["id"].as<std::string>()          : "";
        m.name        = node["name"]        ? node["name"].as<std::string>()        : m.id;
        m.description = node["description"] ? node["description"].as<std::string>() : "";
        m.version     = node["version"]     ? node["version"].as<std::string>()     : "0.0.0";
        m.category    = node["category"]    ? node["category"].as<std::string>()    : "other";
        m.result_type_url = node["result_type_url"] ?
            node["result_type_url"].as<std::string>() : "";
        m.config_type_url = node["config_type_url"] ?
            node["config_type_url"].as<std::string>() : "";

        if (node["modes"] && node["modes"].IsSequence()) {
            for (const auto& mode : node["modes"]) {
                m.modes.push_back(mode.as<std::string>());
            }
        }

        if (m.id.empty()) {
            spdlog::warn("ModuleRegistry: skipping entry with empty id in {}", path);
            continue;
        }

        if (index_.count(m.id)) {
            spdlog::warn("ModuleRegistry: duplicate module_id '{}' in {} — keeping first",
                         m.id, path);
            continue;
        }

        index_[m.id] = modules_.size();
        modules_.push_back(std::move(m));
    }

    spdlog::info("ModuleRegistry: loaded {} module(s) from {}", modules_.size(), path);
}

bool ModuleRegistry::has(std::string_view module_id) const {
    return index_.count(std::string{module_id}) > 0;
}

std::optional<ModuleInfo> ModuleRegistry::find(std::string_view module_id) const {
    auto it = index_.find(std::string{module_id});
    if (it == index_.end()) return std::nullopt;
    return modules_[it->second];
}

}  // namespace zion
