#pragma once
// Settings — all runtime configuration sourced from environment variables.
// Parsed once at startup into a single Settings struct, then shared read-only.

#include <cstdint>
#include <string>
#include <vector>

namespace zion {

struct DbSettings {
    std::string host             = "localhost";
    uint16_t    port             = 5432;
    std::string name             = "zion";
    std::string user             = "zion";
    std::string password;
    uint32_t    pool_size        = 10;
    uint32_t    connect_timeout  = 10;  // seconds
};

struct NetworkSettings {
    std::string grpc_listen    = "0.0.0.0:50051";
    std::string http_listen    = "0.0.0.0:8080";
    std::string metrics_listen = "0.0.0.0:9100";
    bool        metrics_enabled = true;
};

struct ConcurrencySettings {
    uint32_t grpc_threads = 0;   // 0 = hardware_concurrency
    uint32_t http_threads = 0;
};

struct BackpressureSettings {
    uint32_t stream_queue_size = 1024;
    uint32_t log_queue_size    = 4096;
};

struct AuthSettings {
    std::string session_secret;
    uint32_t    argon2_mem_kib  = 65536;
    uint32_t    argon2_iter     = 3;
    uint32_t    argon2_parallel = 4;
    uint32_t    login_rate_limit = 10;   // requests/min per IP
};

struct SmtpSettings {
    std::string host;
    uint16_t    port     = 587;
    std::string user;
    std::string password;
    std::string from     = "zion@localhost";
    bool        tls      = true;
};

struct OtaSettings {
    std::string storage_path    = "/var/lib/zion/ota";
    std::string signing_key_path;   // empty = skip signature verification
};

struct Settings {
    DbSettings          db;
    NetworkSettings     network;
    ConcurrencySettings concurrency;
    BackpressureSettings backpressure;
    AuthSettings        auth;
    SmtpSettings        smtp;
    OtaSettings         ota;
    std::string         log_level       = "info";
    uint32_t            shutdown_timeout = 30;  // seconds
    std::string         registry_path   = "../protos/registry/modules.yaml";
    std::string         cors_origins    = "http://localhost:3000";
    std::vector<std::string> cors_origin_list;  // parsed from cors_origins

    // Convenience helpers
    [[nodiscard]] bool smtp_enabled() const { return !smtp.host.empty(); }
    [[nodiscard]] bool metrics_enabled() const { return network.metrics_enabled && !network.metrics_listen.empty(); }
};

// Load settings from environment variables.  Falls back to defaults for any
// variable that is absent or empty.  Throws std::runtime_error if a required
// variable (e.g. ZION_SESSION_SECRET) is absent or below minimum length.
Settings load_settings();

// Returns a human-readable summary (no secrets) for startup logging.
std::string settings_summary(const Settings& s);

}  // namespace zion
