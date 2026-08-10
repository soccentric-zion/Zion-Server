#pragma once
// rest_api/controllers/TargetsController.h
#include "services/TargetService.h"
#include <drogon/HttpController.h>

namespace zion::rest {

class TargetsController : public drogon::HttpController<TargetsController, false> {
public:
    explicit TargetsController(TargetService& svc);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TargetsController::get_board,    "/api/v1/target",          drogon::Get);
    ADD_METHOD_TO(TargetsController::register_board,"/api/v1/target",         drogon::Post);
    ADD_METHOD_TO(TargetsController::update_board, "/api/v1/target",          drogon::Put);
    ADD_METHOD_TO(TargetsController::delete_board, "/api/v1/target",          drogon::Delete);
    ADD_METHOD_TO(TargetsController::heartbeat,    "/api/v1/target/heartbeat",drogon::Post);
    METHOD_LIST_END

    void get_board(const drogon::HttpRequestPtr&,
                   std::function<void(const drogon::HttpResponsePtr&)>&&);
    void register_board(const drogon::HttpRequestPtr&,
                        std::function<void(const drogon::HttpResponsePtr&)>&&);
    void update_board(const drogon::HttpRequestPtr&,
                      std::function<void(const drogon::HttpResponsePtr&)>&&);
    void delete_board(const drogon::HttpRequestPtr&,
                      std::function<void(const drogon::HttpResponsePtr&)>&&);
    void heartbeat(const drogon::HttpRequestPtr&,
                   std::function<void(const drogon::HttpResponsePtr&)>&&);

private:
    TargetService& svc_;
};

}  // namespace zion::rest
