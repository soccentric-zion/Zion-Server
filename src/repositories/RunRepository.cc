#include "repositories/RunRepository.h"
#include "db/DbClient.h"
#include "util/Errors.h"

#include <drogon/orm/DbClient.h>
#include <spdlog/spdlog.h>

#include <sstream>

namespace zion {

namespace {
Run row_to_run(const drogon::orm::Row& r) {
    Run run;
    run.id                        = r["id"].as<std::string>();
    run.name                      = r["name"].as<std::string>();
    run.description               = r["description"].as<std::string>();
    run.test_config_snapshot_json = r["test_config_snapshot"].as<std::string>();
    run.mode                      = r["mode"].as<std::string>();
    run.status                    = r["status"].as<std::string>();
    run.started_at                = r["started_at"].isNull() ? "" : r["started_at"].as<std::string>();
    run.ended_at                  = r["ended_at"].isNull() ? "" : r["ended_at"].as<std::string>();
    run.error_message             = r["error_message"].isNull() ? "" : r["error_message"].as<std::string>();
    run.tags_json                 = r["tags"].as<std::string>();
    run.notes                     = r["notes"].as<std::string>();
    run.baseline_flag             = r["baseline_flag"].as<bool>();
    run.attached_files_json       = r["attached_files"].as<std::string>();
    run.created_at                = r["created_at"].as<std::string>();
    run.created_by                = r["created_by"].isNull() ? "" : r["created_by"].as<std::string>();
    return run;
}
}  // namespace

Run RunRepository::create(const Run& r) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "INSERT INTO runs (name, description, test_config_snapshot, mode, status,"
        "  tags, notes, baseline_flag, attached_files, created_by)"
        " VALUES ($1, $2, $3::JSONB, $4, $5, $6::JSONB, $7, $8, $9::JSONB, $10::UUID)"
        " RETURNING id,  name,  description,  test_config_snapshot::TEXT,  mode,  status, "
        "  started_at::TEXT,  ended_at::TEXT,  error_message,  tags::TEXT,  notes, "
        "  baseline_flag,  attached_files::TEXT,  created_at::TEXT,  created_by::TEXT", 
        r.name.c_str(),  r.description.c_str(), 
        r.test_config_snapshot_json.empty() ? "{}" : r.test_config_snapshot_json.c_str(), 
        r.mode.c_str(),  r.status.empty() ? "queued" : r.status.c_str(), 
        r.tags_json.empty() ? "[]" : r.tags_json.c_str(), 
        r.notes.c_str(),  r.baseline_flag, 
        r.attached_files_json.empty() ? "[]" : r.attached_files_json.c_str(), 
        r.created_by.empty() ? nullptr : r.created_by.c_str());
    return row_to_run(rows[0]);
}

std::optional<Run> RunRepository::find_by_id(const std::string& id) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id,  name,  description,  test_config_snapshot::TEXT,  mode,  status, "
        "  started_at::TEXT,  ended_at::TEXT,  error_message,  tags::TEXT,  notes, "
        "  baseline_flag,  attached_files::TEXT,  created_at::TEXT,  created_by::TEXT"
        " FROM runs WHERE id = $1::UUID", 
        id.c_str());
    if (rows.empty()) return std::nullopt;
    return row_to_run(rows[0]);
}

std::vector<Run> RunRepository::list(const RunFilter& f) {
    auto db = db::get_db_client();
    std::ostringstream sql;
    std::vector<std::string> params;
    int p = 1;

    sql << "SELECT id, name, description, test_config_snapshot::TEXT, mode, status,"
           "  started_at::TEXT, ended_at::TEXT, error_message, tags::TEXT, notes,"
           "  baseline_flag, attached_files::TEXT, created_at::TEXT, created_by::TEXT"
           " FROM runs WHERE 1=1";

    if (!f.status.empty()) { sql << " AND status = $" << p++; params.push_back(f.status); }
    if (!f.mode.empty())   { sql << " AND mode = $"   << p++; params.push_back(f.mode); }
    if (!f.from_ts.empty()){ sql << " AND started_at >= $" << p++; params.push_back(f.from_ts); }
    if (!f.to_ts.empty())  { sql << " AND started_at <= $" << p++; params.push_back(f.to_ts); }
    if (!f.search.empty()) {
        sql << " AND (name ILIKE $" << p << " OR description ILIKE $" << p << ")";
        params.push_back("%" + f.search + "%");
        ++p;
    }

    sql << " ORDER BY created_at DESC LIMIT " << f.limit << " OFFSET " << f.offset;
    // Build the SQL call with variable parameters
    auto rows = [&]() -> drogon::orm::Result {
        switch (params.size()) {
        case 2:  return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str());
        case 3:  return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str(),  params[2].c_str());
        case 4:  return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str(),  params[2].c_str(),  params[3].c_str());
        case 5:  return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str(),  params[2].c_str(),  params[3].c_str(),  params[4].c_str());
        default: return db->execSqlSync(sql.str(),  params[0].c_str(),  params[1].c_str());
            }
    }();

    std::vector<Run> out;
    for (const auto& r : rows) out.push_back(row_to_run(r));
    return out;
}

void RunRepository::update_status(const std::string& id, const std::string& status,
                                  const std::string& error_message) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE runs SET status=$1,  error_message=NULLIF($2,'') WHERE id=$3::UUID", 
        status.c_str(),  error_message.c_str(),  id.c_str());
}

void RunRepository::set_started(const std::string& id) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE runs SET status='running',  started_at=NOW() WHERE id=$1::UUID",  id.c_str());
}

void RunRepository::set_ended(const std::string& id, const std::string& status,
                              const std::string& error_message) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE runs SET status=$1,  ended_at=NOW(),  error_message=NULLIF($2,'')"
        " WHERE id=$3::UUID", 
        status.c_str(),  error_message.c_str(),  id.c_str());
}

void RunRepository::update_notes(const std::string& id, const std::string& notes) {
    auto db = db::get_db_client();
    db->execSqlSync("UPDATE runs SET notes=$1 WHERE id=$2::UUID",  notes.c_str(),  id.c_str());
}

void RunRepository::update_tags(const std::string& id, const std::string& tags_json) {
    auto db = db::get_db_client();
    db->execSqlSync("UPDATE runs SET tags=$1::JSONB WHERE id=$2::UUID",  tags_json.c_str(),  id.c_str());
}

void RunRepository::set_baseline(const std::string& id, const std::string& mode) {
    auto db = db::get_db_client();
    // Clear existing baseline for this mode, then set the new one
    db->execSqlSync(
        "UPDATE runs SET baseline_flag=FALSE WHERE mode=$1 AND baseline_flag=TRUE",  mode.c_str());
    db->execSqlSync(
        "UPDATE runs SET baseline_flag=TRUE WHERE id=$1::UUID",  id.c_str());
}

void RunRepository::remove(const std::string& id) {
    auto db = db::get_db_client();
    db->execSqlSync("DELETE FROM runs WHERE id=$1::UUID",  id.c_str());
}

}  // namespace zion
