#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zion::config {

struct ServerConfig {
    std::string grpc_listen = "0.0.0.0:50051";
    std::string http_listen = "127.0.0.1:8080";
};

struct StorageConfig {
    std::string db_path  = "/var/lib/zion/zion.db";
    std::string logs_dir = "/var/lib/zion/logs";
};

struct IngestConfig {
    uint32_t queue_capacity          = 100000;
    uint32_t batch_commit_rows       = 1000;
    uint32_t batch_commit_interval_ms = 1000;
};

struct LogsConfig {
    std::vector<std::string> fts_sources      = {"hal", "client"};
    uint32_t file_rotation_size_mb            = 50;
    std::string file_compression              = "zstd";
};

struct MetricsConfig {
    bool enabled         = true;
    std::string listen   = "127.0.0.1:9100";
};

struct Config {
    ServerConfig  server;
    StorageConfig storage;
    IngestConfig  ingest;
    LogsConfig    logs;
    MetricsConfig metrics;
};

// Returns a Config populated with compile-time defaults. No file I/O.
Config defaults();

// Parses path as TOML, overlays values onto defaults().
// Throws std::runtime_error on file-not-found or parse failure.
Config load(const std::string& path);

} // namespace zion::config
