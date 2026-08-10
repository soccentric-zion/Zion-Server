#pragma once
// rest_api/controllers/HealthController.h
#include <drogon/HttpController.h>

namespace zion::rest {

class HealthController : public drogon::HttpController<HealthController, false>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::get,     "/api/v1/health",       drogon::Get);
    ADD_METHOD_TO(HealthController::openapi, "/api/v1/openapi.json", drogon::Get);
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void openapi(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};

}  // namespace zion::rest
