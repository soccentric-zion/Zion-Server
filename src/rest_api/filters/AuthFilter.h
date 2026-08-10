#pragma once
// rest_api/filters/AuthFilter.h — Drogon filter that validates session cookie or Bearer token.

#include "services/AuthService.h"
#include <drogon/HttpFilter.h>

namespace zion::rest {

class AuthFilter : public drogon::HttpFilter<AuthFilter> {
public:
    explicit AuthFilter(AuthService& auth_svc);

    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&&      failCb,
                  drogon::FilterChainCallback&& nextCb) override;

private:
    AuthService& auth_svc_;
};

}  // namespace zion::rest
