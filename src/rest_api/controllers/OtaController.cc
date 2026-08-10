#include "rest_api/controllers/OtaController.h"
#include "util/Errors.h"
#include <drogon/HttpResponse.h>
#include <drogon/MultiPart.h>
#include <json/json.h>
#include <spdlog/spdlog.h>

namespace zion::rest {

OtaController::OtaController(OtaService& svc) : svc_(svc) {}

static Json::Value ota_to_json(const OtaUpgrade& u) {
    Json::Value v;
    v["id"]           = u.id;
    v["status"]       = u.status;
    v["progress"]     = u.progress;
    v["artifact_path"]= u.artifact_path;
    v["artifact_hash"]= u.artifact_hash;
    v["from_version"] = u.from_version;
    v["to_version"]   = u.to_version;
    v["created_at"]   = u.created_at;
    v["ended_at"]     = u.ended_at;
    return v;
}

void OtaController::list(const drogon::HttpRequestPtr& /*req*/,
                          std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto upgrades = svc_.list();
    Json::Value arr{Json::arrayValue};
    for (const auto& u : upgrades) arr.append(ota_to_json(u));
    Json::Value resp; resp["upgrades"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void OtaController::get(const drogon::HttpRequestPtr& /*req*/,
                         std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                         std::string id) {
    try {
        cb(drogon::HttpResponse::newHttpJsonResponse(ota_to_json(svc_.get(id))));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void OtaController::upload(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    // Multipart upload: parse the multipart body to extract the uploaded file.
    drogon::MultiPartParser mp;
    if (mp.parse(req) != 0 || mp.getFiles().empty()) {
        Json::Value err; err["error"] = "no file uploaded";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        cb(resp);
        return;
    }
    const auto& f = mp.getFiles()[0];
    // Save the uploaded artifact to a temporary path under the upload dir.
    std::string tmp_name = "ota_" + f.getFileName();
    f.saveAs(tmp_name);
    std::string saved_path = drogon::app().getUploadPath() + "/" + tmp_name;
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto upgrade = svc_.accept_artifact(f.getFileName(), saved_path, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(ota_to_json(upgrade)));
}

void OtaController::trigger(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    try {
        auto upgrade = svc_.trigger_upgrade(id, user_id);
        cb(drogon::HttpResponse::newHttpJsonResponse(ota_to_json(upgrade)));
    } catch (const std::exception& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        cb(resp);
    }
}

void OtaController::rollback(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                               std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto rb = svc_.rollback(id, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(ota_to_json(rb)));
}

}  // namespace zion::rest
