#include "rest_api/controllers/SettingsController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/DbClient.h>
#include <json/json.h>
#include <stdexcept>
#include <string>

namespace zion::rest {

// Helper: read a value from app_settings synchronously via Drogon's default
// DbClient.  Returns the value string, or the provided default if not found.
static std::string read_setting(const std::string& key, const std::string& def = "") {
    auto db = drogon::app().getDbClient();
    if (!db) return def;
    try {
        auto result = db->execSqlSync("SELECT value FROM app_settings WHERE key=$1", key);
        if (result.empty()) return def;
        return result[0]["value"].as<std::string>();
    } catch (...) {
        return def;
    }
}

static void write_setting(const std::string& key, const std::string& value) {
    auto db = drogon::app().getDbClient();
    if (!db) return;
    db->execSqlSync(
        "INSERT INTO app_settings (key, value, updated_at) VALUES ($1,$2,NOW()) "
        "ON CONFLICT (key) DO UPDATE SET value=$2, updated_at=NOW()",
        key, value);
}

void SettingsController::get_retention(
        const drogon::HttpRequestPtr& /*req*/,
        std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    Json::Value resp;
    try {
        resp["retention_days"]  = std::stoi(read_setting("retention_days", "90"));
        resp["max_run_results"] = std::stoi(read_setting("max_run_results", "0"));
    } catch (...) {
        resp["retention_days"]  = 90;
        resp["max_run_results"] = 0;
    }
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void SettingsController::put_retention(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body = req->getJsonObject();
    if (!body) {
        Json::Value err; err["error"] = "invalid JSON";
        auto r = drogon::HttpResponse::newHttpJsonResponse(err);
        r->setStatusCode(drogon::k400BadRequest);
        cb(r); return;
    }

    if (body->isMember("retention_days")) {
        int d = (*body)["retention_days"].asInt();
        if (d < 0) d = 0;
        write_setting("retention_days", std::to_string(d));
    }
    if (body->isMember("max_run_results")) {
        int m = (*body)["max_run_results"].asInt();
        if (m < 0) m = 0;
        write_setting("max_run_results", std::to_string(m));
    }

    Json::Value resp;
    resp["retention_days"]  = std::stoi(read_setting("retention_days", "90"));
    resp["max_run_results"] = std::stoi(read_setting("max_run_results", "0"));
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

}  // namespace zion::rest
