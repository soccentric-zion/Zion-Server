#include "repositories/NotificationRepository.h"
#include "db/DbClient.h"

#include <drogon/orm/DbClient.h>

namespace zion {

namespace {
Notification row_to_notif(const drogon::orm::Row& r) {
    return Notification{
        r["id"].as<std::string>(),
        r["kind"].as<std::string>(),
        r["payload"].as<std::string>(),
        r["read_at"].isNull() ? "" : r["read_at"].as<std::string>(),
        r["created_at"].as<std::string>()
    };
}
}  // namespace

Notification NotificationRepository::create(const std::string& kind,
                                              const std::string& payload_json) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "INSERT INTO notifications (kind, payload)"
        " VALUES ($1, $2::JSONB)"
        " RETURNING id,  kind,  payload::TEXT,  read_at::TEXT,  created_at::TEXT", 
        kind.c_str(),  payload_json.empty() ? "{}" : payload_json.c_str());
    return row_to_notif(rows[0]);
}

std::vector<Notification> NotificationRepository::list(const NotificationFilter& f) {
    auto db = db::get_db_client();
    std::string sql =
        "SELECT id, kind, payload::TEXT, read_at::TEXT, created_at::TEXT"
        " FROM notifications";
    if (f.unread_only) sql += " WHERE read_at IS NULL";
    sql += " ORDER BY created_at DESC LIMIT $1 OFFSET $2";
    auto rows = db->execSqlSync(sql,  f.limit,  f.offset);
    std::vector<Notification> out;
    for (const auto& r : rows) out.push_back(row_to_notif(r));
    return out;
}

void NotificationRepository::mark_read(const std::string& id) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE notifications SET read_at=NOW() WHERE id=$1::UUID",  id.c_str());
}

void NotificationRepository::mark_all_read() {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE notifications SET read_at=NOW() WHERE read_at IS NULL");
}

int64_t NotificationRepository::unread_count() {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT COUNT(*) FROM notifications WHERE read_at IS NULL");
    return rows[0][0].as<int64_t>();
}

}  // namespace zion
