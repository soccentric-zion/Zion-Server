#pragma once
// rest_api/controllers/AlertsController.h — threshold alert rule management
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <functional>
#include <string>

namespace zion::rest {

/**
 * AlertsController exposes CRUD for alert_rules.
 * Stored in the alert_rules table (migration 0008).
 * Evaluation is wired in RunService::ingest_result().
 */
class AlertsController : public drogon::HttpController<AlertsController, false> {
public:
    AlertsController() = default;

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AlertsController::list,   "/api/v1/alerts",       drogon::Get);
    ADD_METHOD_TO(AlertsController::create, "/api/v1/alerts",       drogon::Post);
    ADD_METHOD_TO(AlertsController::get,    "/api/v1/alerts/{id}",  drogon::Get);
    ADD_METHOD_TO(AlertsController::update, "/api/v1/alerts/{id}",  drogon::Put);
    ADD_METHOD_TO(AlertsController::remove, "/api/v1/alerts/{id}",  drogon::Delete);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr&,
              std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);
    void get(const drogon::HttpRequestPtr&,
             std::function<void(const drogon::HttpResponsePtr&)>&&,
             std::string id);
    void update(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&,
                std::string id);
    void remove(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&,
                std::string id);
};

}  // namespace zion::rest
