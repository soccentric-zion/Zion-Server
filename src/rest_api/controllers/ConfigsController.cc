#include "rest_api/controllers/ConfigsController.h"
#include "util/Errors.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace zion::rest {

ConfigsController::ConfigsController(ConfigService& svc) : svc_(svc) {}

static Json::Value cfg_to_json(const TestConfig& c) {
    Json::Value v;
    v["id"]          = c.id;
    v["name"]        = c.name;
    v["description"] = c.description;
    v["mode"]        = c.mode;
    v["common_config"] = c.common_config_json.empty() ? "{}" : c.common_config_json;
    v["modules"]       = c.modules_json.empty() ? "[]" : c.modules_json;
    v["archived"]    = c.is_archived;
    v["created_at"]  = c.created_at;
    return v;
}

void ConfigsController::list(const drogon::HttpRequestPtr& /*req*/,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto cfgs = svc_.list();
    Json::Value arr{Json::arrayValue};
    for (const auto& c : cfgs) arr.append(cfg_to_json(c));
    Json::Value resp; resp["configs"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void ConfigsController::create(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body = req->getJsonObject();
    TestConfig c;
    if (body) {
        c.name        = (*body)["name"].asString();
        c.description = (*body)["description"].asString();
        c.mode        = (*body)["mode"].asString();
        c.common_config_json = (*body).isMember("common_config") ? (*body)["common_config"].toStyledString() : "{}";
        c.modules_json = (*body).isMember("modules") ? (*body)["modules"].toStyledString() : "[]";
    }
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto created = svc_.create(c, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(cfg_to_json(created)));
}

void ConfigsController::get(const drogon::HttpRequestPtr& /*req*/,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             std::string id) {
    try {
        auto c = svc_.get(id);
        cb(drogon::HttpResponse::newHttpJsonResponse(cfg_to_json(c)));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void ConfigsController::update(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                std::string id) {
    auto body = req->getJsonObject();
    TestConfig c;
    c.id = id;
    if (body) {
        c.name        = (*body)["name"].asString();
        c.description = (*body)["description"].asString();
        c.mode        = (*body)["mode"].asString();
        c.common_config_json = (*body).isMember("common_config") ? (*body)["common_config"].toStyledString() : "{}";
        c.modules_json = (*body).isMember("modules") ? (*body)["modules"].toStyledString() : "[]";
    }
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto updated = svc_.update(c, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(cfg_to_json(updated)));
}

void ConfigsController::remove(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.remove(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void ConfigsController::archive(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                 std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.archive(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

}  // namespace zion::rest
