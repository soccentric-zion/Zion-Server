#include "config/Settings.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace zion {

namespace {

// Returns the env-var value or default_val if absent / empty.
std::string env_str(const char* var, std::string default_val = "") {
    const char* v = std::getenv(var);
    if (!v || *v == '\0') return default_val;
    return v;
}

uint16_t env_u16(const char* var, uint16_t def) {
    const char* v = std::getenv(var);
    if (!v || *v == '\0') return def;
    return static_cast<uint16_t>(std::stoul(v));
}

uint32_t env_u32(const char* var, uint32_t def) {
    const char* v = std::getenv(var);
    if (!v || *v == '\0') return def;
    return static_cast<uint32_t>(std::stoul(v));
}

bool env_bool(const char* var, bool def) {
    const char* v = std::getenv(var);
    if (!v || *v == '\0') return def;
    std::string s{v};
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s == "true" || s == "1" || s == "yes";
}

std::vector<std::string> split_csv(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream ss{s};
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        // trim whitespace
        tok.erase(0, tok.find_first_not_of(" \t"));
        tok.erase(tok.find_last_not_of(" \t") + 1);
        if (!tok.empty()) out.push_back(std::move(tok));
    }
    return out;
}

}  // namespace

Settings load_settings() {
    Settings s;

    // Database
    s.db.host            = env_str("ZION_DB_HOST",     "localhost");
    s.db.port            = env_u16("ZION_DB_PORT",     5432);
    s.db.name            = env_str("ZION_DB_NAME",     "zion");
    s.db.user            = env_str("ZION_DB_USER",     "zion");
    s.db.password        = env_str("ZION_DB_PASSWORD", "");
    s.db.pool_size       = env_u32("ZION_DB_POOL_SIZE", 10);
    s.db.connect_timeout = env_u32("ZION_DB_CONNECT_TIMEOUT_SEC", 10);

    // Network
    s.network.grpc_listen    = env_str("ZION_GRPC_LISTEN",    "0.0.0.0:50051");
    s.network.http_listen    = env_str("ZION_HTTP_LISTEN",    "0.0.0.0:8080");
    s.network.metrics_listen = env_str("ZION_METRICS_LISTEN", "0.0.0.0:9100");
    s.network.metrics_enabled = !s.network.metrics_listen.empty();

    // Concurrency
    auto hw = std::thread::hardware_concurrency();
    s.concurrency.grpc_threads = env_u32("ZION_GRPC_THREADS", 0);
    s.concurrency.http_threads = env_u32("ZION_HTTP_THREADS", 0);
    if (s.concurrency.grpc_threads == 0) s.concurrency.grpc_threads = hw ? hw : 4;
    if (s.concurrency.http_threads == 0) s.concurrency.http_threads = hw ? hw : 4;

    // Backpressure
    s.backpressure.stream_queue_size = env_u32("ZION_STREAM_QUEUE_SIZE", 1024);
    s.backpressure.log_queue_size    = env_u32("ZION_LOG_QUEUE_SIZE",    4096);

    // Auth
    s.auth.session_secret  = env_str("ZION_SESSION_SECRET", "");
    s.auth.argon2_mem_kib  = env_u32("ZION_ARGON2_MEM_KIB", 65536);
    s.auth.argon2_iter     = env_u32("ZION_ARGON2_ITER",    3);
    s.auth.argon2_parallel = env_u32("ZION_ARGON2_PARALLEL", 4);
    s.auth.login_rate_limit = env_u32("ZION_LOGIN_RATE_LIMIT", 10);

    if (s.auth.session_secret.size() < 32) {
        throw std::runtime_error(
            "ZION_SESSION_SECRET must be set and at least 32 characters long");
    }

    // SMTP
    s.smtp.host     = env_str("ZION_SMTP_HOST",     "");
    s.smtp.port     = env_u16("ZION_SMTP_PORT",     587);
    s.smtp.user     = env_str("ZION_SMTP_USER",     "");
    s.smtp.password = env_str("ZION_SMTP_PASSWORD", "");
    s.smtp.from     = env_str("ZION_SMTP_FROM",     "zion@localhost");
    s.smtp.tls      = env_bool("ZION_SMTP_TLS",     true);

    // OTA
    s.ota.storage_path     = env_str("ZION_OTA_STORAGE_PATH",    "/var/lib/zion/ota");
    s.ota.signing_key_path = env_str("ZION_OTA_SIGNING_KEY_PATH", "");

    // Misc
    s.log_level        = env_str("ZION_LOG_LEVEL",           "info");
    s.shutdown_timeout = env_u32("ZION_SHUTDOWN_TIMEOUT_SEC", 30);
    s.registry_path    = env_str("ZION_REGISTRY_PATH",       "../protos/registry/modules.yaml");
    s.cors_origins     = env_str("ZION_CORS_ORIGINS",        "http://localhost:3000");
    s.cors_origin_list = split_csv(s.cors_origins);

    return s;
}

std::string settings_summary(const Settings& s) {
    std::ostringstream o;
    o << "grpc=" << s.network.grpc_listen
      << " http=" << s.network.http_listen
      << " metrics=" << (s.metrics_enabled() ? s.network.metrics_listen : "disabled")
      << " db=" << s.db.user << "@" << s.db.host << ":" << s.db.port << "/" << s.db.name
      << " grpc_threads=" << s.concurrency.grpc_threads
      << " http_threads=" << s.concurrency.http_threads
      << " log=" << s.log_level
      << " registry=" << s.registry_path
      << " smtp=" << (s.smtp_enabled() ? s.smtp.host : "disabled");
    return o.str();
}

}  // namespace zion
