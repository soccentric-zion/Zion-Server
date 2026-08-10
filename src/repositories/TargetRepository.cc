#include "repositories/TargetRepository.h"
#include "db/DbClient.h"
#include "util/Errors.h"

#include <drogon/orm/DbClient.h>

namespace zion {

namespace {
Board row_to_board(const drogon::orm::Row& r) {
    return Board{
        r["id"].as<std::string>(),
        r["name"].as<std::string>(),
        r["hostname"].isNull() ? "" : r["hostname"].as<std::string>(),
        r["model"].isNull() ? "" : r["model"].as<std::string>(),
        r["os_version"].isNull() ? "" : r["os_version"].as<std::string>(),
        r["hal_version"].isNull() ? "" : r["hal_version"].as<std::string>(),
        r["client_version"].isNull() ? "" : r["client_version"].as<std::string>(),
        r["tags"].as<std::string>(),
        r["hal_module_inventory"].as<std::string>(),
        r["first_seen"].as<std::string>(),
        r["last_seen"].as<std::string>(),
        r["status"].as<std::string>()
    };
}
}  // namespace

std::optional<Board> TargetRepository::get() {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id,  name,  hostname,  model,  os_version,  hal_version,  client_version, "
        "  tags::TEXT,  hal_module_inventory::TEXT, "
        "  first_seen::TEXT,  last_seen::TEXT,  status"
        " FROM board LIMIT 1");
    if (rows.empty()) return std::nullopt;
    return row_to_board(rows[0]);
}

Board TargetRepository::create(const Board& b) {
    auto db = db::get_db_client();
    try {
        auto rows = db->execSqlSync(
            "INSERT INTO board (name, hostname, model, os_version, hal_version,"
            "  client_version, tags, hal_module_inventory, status)"
            " VALUES ($1, $2, $3, $4, $5, $6, $7::JSONB, $8::JSONB, $9)"
            " RETURNING id,  name,  hostname,  model,  os_version,  hal_version,  client_version, "
            "  tags::TEXT,  hal_module_inventory::TEXT, "
            "  first_seen::TEXT,  last_seen::TEXT,  status", 
            b.name.c_str(),  b.hostname.c_str(),  b.model.c_str(),  b.os_version.c_str(),  b.hal_version.c_str(), 
            b.client_version.c_str(),  b.tags_json.empty() ? "{}" : b.tags_json.c_str(), 
            b.hal_module_inventory_json.empty() ? "[]" : b.hal_module_inventory_json.c_str(), 
            b.status.empty() ? "offline" : b.status.c_str());
        return row_to_board(rows[0]);
    } catch (const drogon::orm::DrogonDbException& e) {
        std::string what{e.base().what()};
        if (what.find("unique") != std::string::npos) {
            throw ConflictError("board already registered");
        }
        throw;
    }
}

void TargetRepository::update(const Board& b) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE board SET name=$1,  hostname=$2,  tags=$3::JSONB WHERE id=$4::UUID", 
        b.name.c_str(),  b.hostname.c_str(),  b.tags_json.c_str(),  b.id.c_str());
}

void TargetRepository::heartbeat(const std::string& id, const std::string& status) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE board SET last_seen=NOW(),  status=$1 WHERE id=$2::UUID", 
        status.c_str(),  id.c_str());
}

void TargetRepository::update_inventory(const std::string& id,
                                        const std::string& inventory_json) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE board SET hal_module_inventory=$1::JSONB WHERE id=$2::UUID", 
        inventory_json.c_str(),  id.c_str());
}

void TargetRepository::remove() {
    auto db = db::get_db_client();
    db->execSqlSync("DELETE FROM board");
}

}  // namespace zion
