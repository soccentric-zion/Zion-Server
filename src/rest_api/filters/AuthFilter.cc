#include "rest_api/filters/AuthFilter.h"
#include "util/Errors.h"

#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>

namespace zion::rest {

AuthFilter::AuthFilter(AuthService& auth_svc) : auth_svc_(auth_svc) {}

void AuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                           drogon::FilterCallback&&      failCb,
                           drogon::FilterChainCallback&& nextCb) {
    // 1. Check Bearer token header.
    std::string auth_header = req->getHeader("Authorization");
    if (!auth_header.empty() && auth_header.rfind("Bearer ", 0) == 0) {
        std::string token = auth_header.substr(7);
        try {
            auto user_id_opt = auth_svc_.authenticate_token(token);
            if (user_id_opt) {
                req->getAttributes()->insert("user_id", *user_id_opt);
                nextCb();
                return;
            }
        } catch (const AuthError&) {}
    }

    // 2. Check session cookie.
    auto session = req->getSession();
    if (session) {
        std::string user_id = session->get<std::string>("user_id");
        if (!user_id.empty()) {
            req->getAttributes()->insert("user_id", user_id);
            nextCb();
            return;
        }
    }

    // Not authenticated.
    Json::Value err_body;
    err_body["error"] = "Unauthorized";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(err_body);
    resp->setStatusCode(drogon::k401Unauthorized);
    failCb(resp);
}

}  // namespace zion::rest
