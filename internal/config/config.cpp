#include "internal/config/config.hpp"

#include <toml++/toml.hpp>

#include <filesystem>
#include <stdexcept>

namespace zion::config {

Config defaults() {
    return Config{};
}

Config load(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("config file not found: " + path);
    }

    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error(std::string("config parse error: ") + e.what());
    }

    Config cfg;

    // Helper lambdas — silently skip keys that are absent or the wrong type.
    auto str  = [&](auto&& node, std::string& field) {
        if (auto v = node.template value<std::string>()) field = *v;
    };
    auto u32  = [&](auto&& node, uint32_t& field) {
        if (auto v = node.template value<int64_t>()) field = static_cast<uint32_t>(*v);
    };
    auto flag = [&](auto&& node, bool& field) {
        if (auto v = node.template value<bool>()) field = *v;
    };

    str (tbl["server" ]["grpc_listen"],            cfg.server.grpc_listen);
    str (tbl["server" ]["http_listen"],            cfg.server.http_listen);
    str (tbl["storage"]["db_path"],                cfg.storage.db_path);
    str (tbl["storage"]["logs_dir"],               cfg.storage.logs_dir);
    u32 (tbl["ingest" ]["queue_capacity"],         cfg.ingest.queue_capacity);
    u32 (tbl["ingest" ]["batch_commit_rows"],      cfg.ingest.batch_commit_rows);
    u32 (tbl["ingest" ]["batch_commit_interval_ms"], cfg.ingest.batch_commit_interval_ms);
    u32 (tbl["logs"   ]["file_rotation_size_mb"],  cfg.logs.file_rotation_size_mb);
    str (tbl["logs"   ]["file_compression"],       cfg.logs.file_compression);
    flag(tbl["metrics"]["enabled"],               cfg.metrics.enabled);
    str (tbl["metrics"]["listen"],                cfg.metrics.listen);

    if (auto* arr = tbl["logs"]["fts_sources"].as_array()) {
        cfg.logs.fts_sources.clear();
        for (auto& el : *arr) {
            if (auto sv = el.value<std::string>()) {
                cfg.logs.fts_sources.push_back(*sv);
            }
        }
    }

    return cfg;
}

} // namespace zion::config
