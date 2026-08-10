#include "rest_api/controllers/AuthController.h"
#include "util/Errors.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <spdlog/spdlog.h>

namespace zion::rest {

static Json::Value err_json(const std::string& msg) {
    Json::Value v;
    v["error"] = msg;
    return v;
}

AuthController::AuthController(AuthService& svc) : svc_(svc) {}

void AuthController::login(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body = req->getJsonObject();
    if (!body) {
        cb(drogon::HttpResponse::newHttpJsonResponse(err_json("invalid json")));
        return;
    }
    auto username = (*body)["username"].asString();
    auto password = (*body)["password"].asString();
    try {
        auto result = svc_.login(username, password, "");
        if (!result) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err_json("invalid credentials"));
            resp->setStatusCode(drogon::k401Unauthorized);
            cb(resp);
            return;
        }
        auto session = req->getSession();
        session->insert("user_id", result->user.id);
        Json::Value resp;
        resp["user_id"]  = result->user.id;
        resp["username"] = result->user.username;
        cb(drogon::HttpResponse::newHttpJsonResponse(resp));
    } catch (const AuthError& e) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err_json(e.what()));
        resp->setStatusCode(drogon::k401Unauthorized);
        cb(resp);
    }
}

void AuthController::logout(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    req->getSession()->clear();
    Json::Value resp;
    resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void AuthController::create_token(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto body = req->getJsonObject();
    std::string label = body ? (*body)["label"].asString() : "default";
    auto [token_rec, token_str] = svc_.create_api_token(user_id, label);
    Json::Value resp;
    resp["token"] = token_str;   // only shown once
    resp["id"]    = token_rec.id;
    resp["label"] = token_rec.name;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void AuthController::list_tokens(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto tokens = svc_.list_tokens(user_id);
    Json::Value arr{Json::arrayValue};
    for (const auto& t : tokens) {
        Json::Value v;
        v["id"]    = t.id;
        v["label"] = t.name;
        arr.append(v);
    }
    Json::Value resp;
    resp["tokens"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void AuthController::revoke_token(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                   std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.revoke_token(id, user_id);
    Json::Value resp;
    resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void AuthController::change_password(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto body = req->getJsonObject();
    if (!body) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err_json("invalid json"));
        resp->setStatusCode(drogon::k400BadRequest);
        cb(resp);
        return;
    }
    std::string old_password = (*body)["current_password"].asString();
    std::string new_password = (*body)["new_password"].asString();
    try {
        svc_.change_password(user_id, old_password, new_password);
        Json::Value resp; resp["ok"] = true;
        cb(drogon::HttpResponse::newHttpJsonResponse(resp));
    } catch (const AuthError& e) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err_json(e.what()));
        resp->setStatusCode(drogon::k401Unauthorized);
        cb(resp);
    }
}

}  // namespace zion::rest
