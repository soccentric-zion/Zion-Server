#pragma once
// rest_api/controllers/LogsController.h — log listing, SSE live tail, global query, bookmarks
#include "services/LogService.h"
#include <drogon/HttpController.h>

namespace zion::rest {

class LogsController : public drogon::HttpController<LogsController, false> {
public:
    explicit LogsController(LogService& svc);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(LogsController::list,             "/api/v1/runs/{run_id}/logs",       drogon::Get);
    ADD_METHOD_TO(LogsController::tail,             "/api/v1/runs/{run_id}/logs/tail",  drogon::Get);
    ADD_METHOD_TO(LogsController::global_list,      "/api/v1/logs",                     drogon::Get);
    ADD_METHOD_TO(LogsController::search,           "/api/v1/logs/search",              drogon::Get);
    ADD_METHOD_TO(LogsController::list_bookmarks,   "/api/v1/logs/bookmarks",           drogon::Get);
    ADD_METHOD_TO(LogsController::create_bookmark,  "/api/v1/logs/bookmarks",           drogon::Post);
    ADD_METHOD_TO(LogsController::delete_bookmark,  "/api/v1/logs/bookmarks/{id}",      drogon::Delete);
    ADD_METHOD_TO(LogsController::list_views,       "/api/v1/logs/views",               drogon::Get);
    ADD_METHOD_TO(LogsController::create_view,      "/api/v1/logs/views",               drogon::Post);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr&,
              std::function<void(const drogon::HttpResponsePtr&)>&&,
              std::string run_id);
    void tail(const drogon::HttpRequestPtr&,
              std::function<void(const drogon::HttpResponsePtr&)>&&,
              std::string run_id);
    void global_list(const drogon::HttpRequestPtr&,
                     std::function<void(const drogon::HttpResponsePtr&)>&&);
    void search(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);
    void list_bookmarks(const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create_bookmark(const drogon::HttpRequestPtr&,
                         std::function<void(const drogon::HttpResponsePtr&)>&&);
    void delete_bookmark(const drogon::HttpRequestPtr&,
                         std::function<void(const drogon::HttpResponsePtr&)>&&,
                         std::string id);
    void list_views(const drogon::HttpRequestPtr&,
                    std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create_view(const drogon::HttpRequestPtr&,
                     std::function<void(const drogon::HttpResponsePtr&)>&&);

private:
    LogService& svc_;
};

}  // namespace zion::rest
