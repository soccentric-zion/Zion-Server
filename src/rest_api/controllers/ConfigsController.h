#pragma once
// rest_api/controllers/ConfigsController.h
#include "services/ConfigService.h"
#include <drogon/HttpController.h>

namespace zion::rest {

class ConfigsController : public drogon::HttpController<ConfigsController, false> {
public:
    explicit ConfigsController(ConfigService& svc);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ConfigsController::list,    "/api/v1/configs",          drogon::Get);
    ADD_METHOD_TO(ConfigsController::create,  "/api/v1/configs",          drogon::Post);
    ADD_METHOD_TO(ConfigsController::get,     "/api/v1/configs/{id}",     drogon::Get);
    ADD_METHOD_TO(ConfigsController::update,  "/api/v1/configs/{id}",     drogon::Put);
    ADD_METHOD_TO(ConfigsController::remove,  "/api/v1/configs/{id}",     drogon::Delete);
    ADD_METHOD_TO(ConfigsController::archive, "/api/v1/configs/{id}/archive", drogon::Post);
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

private:
    ConfigService& svc_;
};

}  // namespace zion::rest
