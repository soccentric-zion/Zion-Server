#include "db/DbClient.h"

#include "config/Settings.h"

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

namespace zion::db {

namespace {
drogon::orm::DbClientPtr g_db_client;
}

std::string build_dsn(const DbSettings& cfg) {
    // Drogon expects: "host=... port=... dbname=... user=... password=..."
    std::string dsn;
    dsn += "host=" + cfg.host;
    dsn += " port=" + std::to_string(cfg.port);
    dsn += " dbname=" + cfg.name;
    dsn += " user=" + cfg.user;
    if (!cfg.password.empty()) {
        dsn += " password=" + cfg.password;
    }
    dsn += " connect_timeout=" + std::to_string(cfg.connect_timeout);
    return dsn;
}

void init_db_client(const DbSettings& cfg) {
    std::string dsn = build_dsn(cfg);
    spdlog::info("db: connecting to {}@{}:{}/{} pool={}",
                 cfg.user, cfg.host, cfg.port, cfg.name, cfg.pool_size);

    g_db_client = drogon::orm::DbClient::newPgClient(
        dsn,
        static_cast<size_t>(cfg.pool_size));

    if (!g_db_client) {
        throw std::runtime_error("db: failed to create Drogon DbClient");
    }
}

drogon::orm::DbClientPtr get_db_client() {
    if (!g_db_client) {
        throw std::runtime_error("db: DbClient not initialised — call init_db_client() first");
    }
    return g_db_client;
}

}  // namespace zion::db
