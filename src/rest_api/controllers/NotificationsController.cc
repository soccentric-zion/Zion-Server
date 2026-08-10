#include "rest_api/controllers/NotificationsController.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace zion::rest {

NotificationsController::NotificationsController(NotificationService& svc) : svc_(svc) {}

void NotificationsController::list(const drogon::HttpRequestPtr& /*req*/,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    NotificationFilter f;
    auto items = svc_.list(f);
    Json::Value arr{Json::arrayValue};
    for (const auto& n : items) {
        Json::Value v;
        v["id"]      = n.id;
        v["kind"]    = n.kind;
        v["payload"] = n.payload_json;
        v["read"]    = !n.read_at.empty();
        arr.append(v);
    }
    Json::Value resp; resp["notifications"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void NotificationsController::mark_read(const drogon::HttpRequestPtr& /*req*/,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                         std::string id) {
    svc_.mark_read(id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void NotificationsController::mark_all(const drogon::HttpRequestPtr& /*req*/,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    svc_.mark_all_read();
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void NotificationsController::unread_count(const drogon::HttpRequestPtr& /*req*/,
                                            std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    Json::Value resp;
    resp["count"] = static_cast<Json::Int64>(svc_.unread_count());
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

}  // namespace zion::rest
