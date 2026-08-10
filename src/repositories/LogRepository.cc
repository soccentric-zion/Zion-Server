#include "repositories/LogRepository.h"
#include "db/DbClient.h"

#include <drogon/orm/DbClient.h>

namespace zion {

namespace {
LogEntry row_to_log(const drogon::orm::Row& r) {
    return LogEntry{
        r["id"].as<std::string>(),
        r["source"].as<std::string>(),
        r["severity"].as<std::string>(),
        r["timestamp"].as<std::string>(),
        r["message"].as<std::string>(),
        r["metadata"].as<std::string>(),
        r["run_id"].isNull() ? "" : r["run_id"].as<std::string>()
    };
}
}  // namespace

LogEntry LogRepository::insert(const LogEntry& e) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "INSERT INTO log_entries (source, severity, message, metadata, run_id)"
        " VALUES ($1, $2, $3, $4::JSONB, $5::UUID)"
        " RETURNING id,  source,  severity,  timestamp::TEXT,  message,  metadata::TEXT, "
        "  run_id::TEXT", 
        e.source.c_str(),  e.severity.c_str(),  e.message.c_str(), 
        e.metadata_json.empty() ? "{}" : e.metadata_json.c_str(), 
        e.run_id.empty() ? nullptr : e.run_id.c_str());
    return row_to_log(rows[0]);
}

std::vector<LogEntry> LogRepository::list(const LogFilter& f) {
    auto db = db::get_db_client();
    std::ostringstream sql;
    std::vector<std::string> params;
    int p = 1;
    sql << "SELECT id, source, severity, timestamp::TEXT, message, metadata::TEXT,"
           "  run_id::TEXT FROM log_entries WHERE 1=1";

    if (!f.source.empty())   { sql << " AND source=$"   << p++; params.push_back(f.source); }
    if (!f.severity.empty()) { sql << " AND severity=$" << p++; params.push_back(f.severity); }
    if (!f.from_ts.empty())  { sql << " AND timestamp>=$"<< p++; params.push_back(f.from_ts); }
    if (!f.to_ts.empty())    { sql << " AND timestamp<=$"<< p++; params.push_back(f.to_ts); }
    if (!f.run_id.empty())   { sql << " AND run_id=$"   << p++ << "::UUID"; params.push_back(f.run_id); }
    if (!f.query.empty())    {
        sql << " AND to_tsvector('english', message) @@ plainto_tsquery('english', $" << p++ << ")";
        params.push_back(f.query);
    }

    sql << " ORDER BY timestamp DESC LIMIT " << f.limit << " OFFSET " << f.offset;
    auto rows = [&]() -> drogon::orm::Result {
        switch (params.size()) {
        case 2: return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str());
        case 3: return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str(),  params[2].c_str());
        case 4: return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str(),  params[2].c_str(),  params[3].c_str());
        case 5: return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str(),  params[2].c_str(),  params[3].c_str(),  params[4].c_str());
        default: return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str());
            }
    }();

    std::vector<LogEntry> out;
    for (const auto& r : rows) out.push_back(row_to_log(r));
    return out;
}

int64_t LogRepository::count(const LogFilter& f) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT COUNT(*) FROM log_entries WHERE 1=1");
    return rows[0][0].as<int64_t>();
}

LogBookmark LogRepository::create_bookmark(const std::string& user_id,
                                            const std::string& log_id,
                                            const std::string& note) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "INSERT INTO log_bookmarks (user_id, log_id, note)"
        " VALUES ($1::UUID, $2::UUID, $3)"
        " RETURNING id,  user_id::TEXT,  log_id::TEXT,  note,  created_at::TEXT", 
        user_id.c_str(),  log_id.c_str(),  note.c_str());
    const auto& r = rows[0];
    return LogBookmark{
        r["id"].as<std::string>(),
        r["user_id"].as<std::string>(),
        r["log_id"].as<std::string>(),
        r["note"].as<std::string>(),
        r["created_at"].as<std::string>()
    };
}

std::vector<LogBookmark> LogRepository::list_bookmarks(const std::string& user_id) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id,  user_id::TEXT,  log_id::TEXT,  note,  created_at::TEXT"
        " FROM log_bookmarks WHERE user_id=$1::UUID ORDER BY created_at DESC", 
        user_id.c_str());
    std::vector<LogBookmark> out;
    for (const auto& r : rows) {
        out.push_back({
            r["id"].as<std::string>(),
            r["user_id"].as<std::string>(),
            r["log_id"].as<std::string>(),
            r["note"].as<std::string>(),
            r["created_at"].as<std::string>()
        });
    }
    return out;
}

void LogRepository::remove_bookmark(const std::string& id) {
    auto db = db::get_db_client();
    db->execSqlSync("DELETE FROM log_bookmarks WHERE id=$1::UUID",  id.c_str());
}

SavedLogView LogRepository::create_view(const std::string& user_id,
                                         const std::string& name,
                                         const std::string& filters_json) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "INSERT INTO saved_log_views (user_id, name, filters)"
        " VALUES ($1::UUID, $2, $3::JSONB)"
        " RETURNING id,  user_id::TEXT,  name,  filters::TEXT,  created_at::TEXT", 
        user_id.c_str(),  name.c_str(),  filters_json.empty() ? "{}" : filters_json.c_str());
    const auto& r = rows[0];
    return SavedLogView{
        r["id"].as<std::string>(),
        r["user_id"].as<std::string>(),
        r["name"].as<std::string>(),
        r["filters"].as<std::string>(),
        r["created_at"].as<std::string>()
    };
}

std::vector<SavedLogView> LogRepository::list_views(const std::string& user_id) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id,  user_id::TEXT,  name,  filters::TEXT,  created_at::TEXT"
        " FROM saved_log_views WHERE user_id=$1::UUID ORDER BY created_at DESC", 
        user_id.c_str());
    std::vector<SavedLogView> out;
    for (const auto& r : rows) {
        out.push_back({
            r["id"].as<std::string>(),
            r["user_id"].as<std::string>(),
            r["name"].as<std::string>(),
            r["filters"].as<std::string>(),
            r["created_at"].as<std::string>()
        });
    }
    return out;
}

}  // namespace zion
