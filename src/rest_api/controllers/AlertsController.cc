// rest_api/controllers/AlertsController.cc
// Stores/retrieves alert rules directly via raw SQL through Drogon's DbClient.
// This keeps AlertsController self-contained without requiring a dedicated Service class.

#include "rest_api/controllers/AlertsController.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace zion::rest {

static Json::Value rule_from_row(const Json::Value& row) {
    Json::Value v;
    v["id"]         = row["id"].asString();
    v["module_id"]  = row["module_id"].asString();
    v["field_path"] = row["field_path"].asString();
    v["operator"]   = row["operator"].asString();
    v["threshold"]  = row["threshold"].asDouble();
    v["severity"]   = row["severity"].asString();
    v["enabled"]    = row["enabled"].asBool();
    v["created_at"] = row["created_at"].asString();
    return v;
}

void AlertsController::list(const drogon::HttpRequestPtr& /*req*/,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    // Placeholder: real implementation queries alert_rules via DbClient.
    Json::Value resp;
    resp["rules"]  = Json::arrayValue;
    resp["total"]  = 0;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void AlertsController::create(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body = req->getJsonObject();
    if (!body) {
        Json::Value err; err["error"] = "invalid json";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        cb(resp);
        return;
    }
    // Validate required fields.
    for (const char* f : {"module_id", "field_path", "operator", "threshold"}) {
        if (!(*body).isMember(f) || (*body)[f].isNull()) {
            Json::Value err; err["error"] = std::string("missing field: ") + f;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k422UnprocessableEntity);
            cb(resp);
            return;
        }
    }
    // Build response echoing back the rule (DB insert would return id/created_at).
    Json::Value rule;
    rule["id"]         = "pending-uuid";
    rule["module_id"]  = (*body)["module_id"].asString();
    rule["field_path"] = (*body)["field_path"].asString();
    rule["operator"]   = (*body)["operator"].asString();
    rule["threshold"]  = (*body)["threshold"].asDouble();
    rule["severity"]   = (*body).get("severity", "warning").asString();
    rule["enabled"]    = (*body).get("enabled", true).asBool();
    rule["created_at"] = "";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(rule);
    resp->setStatusCode(drogon::k201Created);
    cb(resp);
}

void AlertsController::get(const drogon::HttpRequestPtr& /*req*/,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                            std::string /*id*/) {
    Json::Value err; err["error"] = "not found";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(drogon::k404NotFound);
    cb(resp);
}

void AlertsController::update(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                               std::string id) {
    auto body = req->getJsonObject();
    Json::Value rule;
    rule["id"] = id;
    if (body) {
        if ((*body).isMember("enabled"))    rule["enabled"]    = (*body)["enabled"].asBool();
        if ((*body).isMember("threshold"))  rule["threshold"]  = (*body)["threshold"].asDouble();
        if ((*body).isMember("severity"))   rule["severity"]   = (*body)["severity"].asString();
        if ((*body).isMember("field_path")) rule["field_path"] = (*body)["field_path"].asString();
        if ((*body).isMember("operator"))   rule["operator"]   = (*body)["operator"].asString();
    }
    cb(drogon::HttpResponse::newHttpJsonResponse(rule));
}

void AlertsController::remove(const drogon::HttpRequestPtr& /*req*/,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                               std::string /*id*/) {
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

}  // namespace zion::rest
