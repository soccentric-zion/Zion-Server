#pragma once
// rest_api/controllers/TemplatesController.h — CRUD for run templates
#include "services/TemplateService.h"
#include <drogon/HttpController.h>

namespace zion::rest {

class TemplatesController : public drogon::HttpController<TemplatesController, false> {
public:
    explicit TemplatesController(TemplateService& svc);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TemplatesController::list,        "/api/v1/templates",                    drogon::Get);
    ADD_METHOD_TO(TemplatesController::create,      "/api/v1/templates",                    drogon::Post);
    ADD_METHOD_TO(TemplatesController::get,         "/api/v1/templates/{id}",               drogon::Get);
    ADD_METHOD_TO(TemplatesController::update,      "/api/v1/templates/{id}",               drogon::Put);
    ADD_METHOD_TO(TemplatesController::remove,      "/api/v1/templates/{id}",               drogon::Delete);
    ADD_METHOD_TO(TemplatesController::archive,     "/api/v1/templates/{id}/archive",       drogon::Post);
    ADD_METHOD_TO(TemplatesController::instantiate, "/api/v1/templates/{id}/instantiate",   drogon::Post);
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
    void archive(const drogon::HttpRequestPtr&,
                 std::function<void(const drogon::HttpResponsePtr&)>&&,
                 std::string id);
    void instantiate(const drogon::HttpRequestPtr&,
                     std::function<void(const drogon::HttpResponsePtr&)>&&,
                     std::string id);

private:
    TemplateService& svc_;
};

}  // namespace zion::rest
