#include "rest_api/controllers/SchedulesController.h"
#include "util/Errors.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace zion::rest {

static Json::Value schedule_to_json(const Schedule& s) {
    Json::Value v;
    v["id"]              = s.id;
    v["template_id"]     = s.template_id;
    v["cron_expression"] = s.cron_expression;
    v["run_at"]          = s.run_at;
    v["enabled"]         = s.enabled;
    v["last_run_at"]     = s.last_run_at;
    v["next_run_at"]     = s.next_run_at;
    v["created_at"]      = s.created_at;
    v["created_by"]      = s.created_by;
    return v;
}

SchedulesController::SchedulesController(ScheduleService& svc) : svc_(svc) {}

void SchedulesController::list(const drogon::HttpRequestPtr& /*req*/,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto items = svc_.list_enabled();
    Json::Value arr{Json::arrayValue};
    for (const auto& s : items) arr.append(schedule_to_json(s));
    Json::Value resp;
    resp["schedules"] = arr;
    resp["total"]     = static_cast<int>(items.size());
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void SchedulesController::create(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body    = req->getJsonObject();
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    Schedule s;
    if (body) {
        s.template_id     = (*body)["template_id"].asString();
        s.cron_expression = (*body)["cron_expression"].asString();
        s.run_at          = (*body)["run_at"].asString();
        s.enabled         = (*body).get("enabled", true).asBool();
    }
    auto created = svc_.create(s, user_id);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(schedule_to_json(created));
    resp->setStatusCode(drogon::k201Created);
    cb(resp);
}

void SchedulesController::get(const drogon::HttpRequestPtr& /*req*/,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                               std::string id) {
    try {
        auto s = svc_.get(id);
        cb(drogon::HttpResponse::newHttpJsonResponse(schedule_to_json(s)));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void SchedulesController::update(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                  std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto body = req->getJsonObject();
    Schedule s = svc_.get(id);
    if (body) {
        if ((*body).isMember("template_id"))     s.template_id     = (*body)["template_id"].asString();
        if ((*body).isMember("cron_expression")) s.cron_expression = (*body)["cron_expression"].asString();
        if ((*body).isMember("run_at"))          s.run_at          = (*body)["run_at"].asString();
        if ((*body).isMember("enabled"))         s.enabled         = (*body)["enabled"].asBool();
    }
    auto updated = svc_.update(s, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(schedule_to_json(updated)));
}

void SchedulesController::remove(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                  std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.remove(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void SchedulesController::disable(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                   std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.set_enabled(id, false, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void SchedulesController::enable(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                  std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.set_enabled(id, true, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

}  // namespace zion::rest
