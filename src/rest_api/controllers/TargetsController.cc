#include "rest_api/controllers/TargetsController.h"
#include "util/Errors.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace zion::rest {

TargetsController::TargetsController(TargetService& svc) : svc_(svc) {}

static Json::Value board_to_json(const Board& b) {
    Json::Value v;
    v["id"]             = b.id;
    v["name"]           = b.name;
    v["hostname"]       = b.hostname;
    v["model"]          = b.model;
    v["status"]         = b.status;
    v["first_seen"]     = b.first_seen;
    v["last_seen"]      = b.last_seen;
    v["os_version"]     = b.os_version;
    v["hal_version"]    = b.hal_version;
    v["client_version"] = b.client_version;
    return v;
}

void TargetsController::get_board(const drogon::HttpRequestPtr& /*req*/,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto b = svc_.get_board();
        if (!b) {
            Json::Value err; err["error"] = "no board registered";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k404NotFound);
            cb(resp);
            return;
        }
        cb(drogon::HttpResponse::newHttpJsonResponse(board_to_json(*b)));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void TargetsController::register_board(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body = req->getJsonObject();
    Board b;
    if (body) {
        b.name           = (*body)["name"].asString();
        b.hostname       = (*body)["hostname"].asString();
        b.model          = (*body)["model"].asString();
        b.os_version     = (*body)["os_version"].asString();
        b.hal_version    = (*body)["hal_version"].asString();
        b.client_version = (*body)["client_version"].asString();
    }
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto created = svc_.register_board(b, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(board_to_json(created)));
}

void TargetsController::update_board(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body = req->getJsonObject();
    Board b;
    if (body) {
        b.name           = (*body)["name"].asString();
        b.hostname       = (*body)["hostname"].asString();
        b.model          = (*body)["model"].asString();
        b.os_version     = (*body)["os_version"].asString();
        b.hal_version    = (*body)["hal_version"].asString();
        b.client_version = (*body)["client_version"].asString();
    }
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.update_board(b, user_id);
    auto updated = svc_.get_board();
    if (updated) {
        cb(drogon::HttpResponse::newHttpJsonResponse(board_to_json(*updated)));
    } else {
        cb(drogon::HttpResponse::newHttpJsonResponse(Json::Value{}));
    }
}

void TargetsController::heartbeat(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto board = svc_.get_board();
    if (board) {
        svc_.heartbeat(board->id, "online");
        // Optionally update version fields if provided in heartbeat body.
        auto body = req->getJsonObject();
        if (body && (!(*body)["hal_version"].isNull() || !(*body)["client_version"].isNull())) {
            Board patch;
            patch.os_version     = (*body)["os_version"].asString();
            patch.hal_version    = (*body)["hal_version"].asString();
            patch.client_version = (*body)["client_version"].asString();
            patch.hostname       = (*body)["hostname"].asString();
            // Only update if non-empty to avoid overwriting with empty strings.
            if (!patch.hal_version.empty() || !patch.client_version.empty()) {
                svc_.update_board(patch, "heartbeat");
            }
        }
    }
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void TargetsController::delete_board(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    try {
        svc_.remove_board(user_id);
        Json::Value resp; resp["ok"] = true;
        cb(drogon::HttpResponse::newHttpJsonResponse(resp));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

}  // namespace zion::rest
