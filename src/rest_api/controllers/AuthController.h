#pragma once
// rest_api/controllers/AuthController.h — login, logout, token management
#include "services/AuthService.h"
#include <drogon/HttpController.h>

namespace zion::rest {

class AuthController : public drogon::HttpController<AuthController, false> {
public:
    explicit AuthController(AuthService& svc);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::login,           "/api/v1/auth/login",          drogon::Post);
    ADD_METHOD_TO(AuthController::logout,          "/api/v1/auth/logout",         drogon::Post);
    ADD_METHOD_TO(AuthController::create_token,    "/api/v1/auth/tokens",         drogon::Post);
    ADD_METHOD_TO(AuthController::list_tokens,     "/api/v1/auth/tokens",         drogon::Get);
    ADD_METHOD_TO(AuthController::revoke_token,    "/api/v1/auth/tokens/{id}",    drogon::Delete);
    ADD_METHOD_TO(AuthController::change_password, "/api/v1/auth/password",       drogon::Patch);
    METHOD_LIST_END

    void login(const drogon::HttpRequestPtr&,
               std::function<void(const drogon::HttpResponsePtr&)>&&);
    void logout(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create_token(const drogon::HttpRequestPtr&,
                      std::function<void(const drogon::HttpResponsePtr&)>&&);
    void list_tokens(const drogon::HttpRequestPtr&,
                     std::function<void(const drogon::HttpResponsePtr&)>&&);
    void revoke_token(const drogon::HttpRequestPtr&,
                      std::function<void(const drogon::HttpResponsePtr&)>&&,
                      std::string id);
    void change_password(const drogon::HttpRequestPtr&,
                         std::function<void(const drogon::HttpResponsePtr&)>&&);

private:
    AuthService& svc_;
};

}  // namespace zion::rest
