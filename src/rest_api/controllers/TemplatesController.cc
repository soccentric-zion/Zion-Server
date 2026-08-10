#include "rest_api/controllers/TemplatesController.h"
#include "util/Errors.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <sstream>

namespace zion::rest {

static Json::Value template_to_json(const Template& t) {
    Json::Value v;
    v["id"]          = t.id;
    v["name"]        = t.name;
    v["description"] = t.description;
    v["mode"]        = t.mode;
    v["created_at"]  = t.created_at;
    v["created_by"]  = t.created_by;
    v["is_archived"] = t.is_archived;
    // Embed common_config and modules as parsed JSON objects if available.
    auto parse_json_field = [](const std::string& s, Json::Value& out) {
        if (s.empty()) return;
        Json::CharReaderBuilder rb;
        std::string errs;
        std::istringstream ss(s);
        Json::parseFromStream(rb, ss, &out, &errs);
    };
    Json::Value cc, mods;
    parse_json_field(t.common_config_json, cc);
    parse_json_field(t.modules_json, mods);
    if (!cc.isNull())   v["common_config"] = cc;
    if (!mods.isNull()) v["modules"]       = mods;
    return v;
}

TemplatesController::TemplatesController(TemplateService& svc) : svc_(svc) {}

void TemplatesController::list(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    bool include_archived = (req->getParameter("archived") == "true");
    auto items = svc_.list(include_archived);
    Json::Value arr{Json::arrayValue};
    for (const auto& t : items) arr.append(template_to_json(t));
    Json::Value resp;
    resp["templates"] = arr;
    resp["total"]     = static_cast<int>(items.size());
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void TemplatesController::create(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body    = req->getJsonObject();
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    Template t;
    if (body) {
        t.name             = (*body)["name"].asString();
        t.description      = (*body)["description"].asString();
        t.mode             = (*body)["mode"].asString();
        // Re-serialize sub-objects as JSON strings for storage.
        Json::FastWriter fw;
        if ((*body).isMember("common_config")) t.common_config_json = fw.write((*body)["common_config"]);
        if ((*body).isMember("modules"))       t.modules_json       = fw.write((*body)["modules"]);
    }
    auto created = svc_.create(t, user_id);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(template_to_json(created));
    resp->setStatusCode(drogon::k201Created);
    cb(resp);
}

void TemplatesController::get(const drogon::HttpRequestPtr& /*req*/,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                               std::string id) {
    try {
        auto t = svc_.get(id);
        cb(drogon::HttpResponse::newHttpJsonResponse(template_to_json(t)));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void TemplatesController::update(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                  std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto body = req->getJsonObject();
    Template t = svc_.get(id);
    if (body) {
        Json::FastWriter fw;
        if ((*body).isMember("name"))          t.name             = (*body)["name"].asString();
        if ((*body).isMember("description"))   t.description      = (*body)["description"].asString();
        if ((*body).isMember("mode"))          t.mode             = (*body)["mode"].asString();
        if ((*body).isMember("common_config")) t.common_config_json = fw.write((*body)["common_config"]);
        if ((*body).isMember("modules"))       t.modules_json       = fw.write((*body)["modules"]);
    }
    auto updated = svc_.update(t, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(template_to_json(updated)));
}

void TemplatesController::remove(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                  std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.remove(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void TemplatesController::archive(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                   std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.archive(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void TemplatesController::instantiate(const drogon::HttpRequestPtr& /*req*/,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                       std::string id) {
    // Returns a draft TestConfig JSON derived from this template.
    std::string config_json = svc_.instantiate_to_json(id);
    Json::Value resp;
    resp["config_json"] = config_json;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

}  // namespace zion::rest
