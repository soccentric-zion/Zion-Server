#pragma once
// rest_api/controllers/ModulesController.h
#include "services/ModuleService.h"
#include "services/RunService.h"
#include "services/CompareService.h"
#include <drogon/HttpController.h>

namespace zion::rest {

class ModulesController : public drogon::HttpController<ModulesController, false> {
public:
    ModulesController(ModuleService& svc, RunService& run_svc,
                      CompareService& cmp_svc);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ModulesController::list,          "/api/v1/modules",                drogon::Get);
    ADD_METHOD_TO(ModulesController::get,           "/api/v1/modules/{id}",           drogon::Get);
    ADD_METHOD_TO(ModulesController::trend,         "/api/v1/modules/{id}/trend",     drogon::Get);
    ADD_METHOD_TO(ModulesController::distribution,  "/api/v1/modules/{id}/distribution", drogon::Get);
    ADD_METHOD_TO(ModulesController::compare_runs,  "/api/v1/runs/{a}/compare/{b}",  drogon::Get);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr&,
              std::function<void(const drogon::HttpResponsePtr&)>&&);
    void get(const drogon::HttpRequestPtr&,
             std::function<void(const drogon::HttpResponsePtr&)>&&,
             std::string id);

    // GET /api/v1/modules/{id}/trend?test_id=X&field=Y&from=T&to=T
    void trend(const drogon::HttpRequestPtr&,
               std::function<void(const drogon::HttpResponsePtr&)>&&,
               std::string id);

    // GET /api/v1/modules/{id}/distribution?test_id=X&field=Y&run_ids=a,b,c
    void distribution(const drogon::HttpRequestPtr&,
                      std::function<void(const drogon::HttpResponsePtr&)>&&,
                      std::string id);

    // GET /api/v1/runs/{a}/compare/{b}?drift_pct=5
    void compare_runs(const drogon::HttpRequestPtr&,
                      std::function<void(const drogon::HttpResponsePtr&)>&&,
                      std::string a, std::string b);

private:
    ModuleService&  svc_;
    RunService&     run_svc_;
    CompareService& cmp_svc_;
};

}  // namespace zion::rest
