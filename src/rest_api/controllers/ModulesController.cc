#include "rest_api/controllers/ModulesController.h"
#include "util/Errors.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <sstream>

namespace zion::rest {

ModulesController::ModulesController(ModuleService& svc, RunService& run_svc,
                                      CompareService& cmp_svc)
    : svc_(svc), run_svc_(run_svc), cmp_svc_(cmp_svc) {}

static Json::Value mod_to_json(const ModuleInfo& m) {
    Json::Value v;
    v["id"]              = m.id;
    v["name"]            = m.name;
    v["description"]     = m.description;
    v["version"]         = m.version;
    v["category"]        = m.category;
    v["result_type_url"] = m.result_type_url;
    v["config_type_url"] = m.config_type_url;
    return v;
}

void ModulesController::list(const drogon::HttpRequestPtr& /*req*/,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto mods = svc_.list_all();
    Json::Value arr{Json::arrayValue};
    for (const auto& m : mods) arr.append(mod_to_json(m));
    Json::Value resp;
    resp["modules"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void ModulesController::get(const drogon::HttpRequestPtr& /*req*/,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             std::string id) {
    auto m = svc_.get(id);
    if (!m) {
        Json::Value err; err["error"] = "Not found";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
        return;
    }
    cb(drogon::HttpResponse::newHttpJsonResponse(mod_to_json(*m)));
}

void ModulesController::trend(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
    std::string id) {
    std::string test_id = req->getParameter("test_id");
    std::string field   = req->getParameter("field");
    std::string from_ts = req->getParameter("from");
    std::string to_ts   = req->getParameter("to");
    if (field.empty()) {
        Json::Value err; err["error"] = "field query param required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        cb(resp);
        return;
    }
    auto pts = run_svc_.get_module_trend(id, test_id, field, from_ts, to_ts);
    Json::Value arr{Json::arrayValue};
    for (const auto& p : pts) {
        Json::Value o;
        o["timestamp"] = p.timestamp;
        o["value"]     = p.value;
        arr.append(o);
    }
    Json::Value resp;
    resp["module_id"] = id;
    resp["test_id"]   = test_id;
    resp["field"]     = field;
    resp["trend"]     = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void ModulesController::distribution(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
    std::string id) {
    std::string test_id  = req->getParameter("test_id");
    std::string field    = req->getParameter("field");
    std::string run_ids_str = req->getParameter("run_ids");
    if (field.empty()) {
        Json::Value err; err["error"] = "field query param required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        cb(resp);
        return;
    }
    std::vector<std::string> run_ids;
    if (!run_ids_str.empty()) {
        std::istringstream ss(run_ids_str);
        std::string tok;
        while (std::getline(ss, tok, ',')) if (!tok.empty()) run_ids.push_back(tok);
    }
    auto vals = run_svc_.get_distribution(id, test_id, field, run_ids);
    Json::Value arr{Json::arrayValue};
    for (double v : vals) arr.append(v);
    Json::Value resp;
    resp["module_id"]   = id;
    resp["test_id"]     = test_id;
    resp["field"]       = field;
    resp["distribution"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void ModulesController::compare_runs(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& cb,
    std::string a, std::string b) {
    double drift_pct = 5.0;
    auto dp = req->getParameter("drift_pct");
    if (!dp.empty()) drift_pct = std::stod(dp);
    auto entries = cmp_svc_.compare(a, b, drift_pct);
    Json::Value arr{Json::arrayValue};
    for (const auto& e : entries) {
        Json::Value o;
        o["module_id"]    = e.module_id;
        o["test_id"]      = e.test_id;
        o["classification"] = e.classification;
        if (e.run_a_status.has_value()) o["run_a_status"] = *e.run_a_status;
        else o["run_a_status"] = Json::nullValue;
        if (e.run_b_status.has_value()) o["run_b_status"] = *e.run_b_status;
        else o["run_b_status"] = Json::nullValue;
        arr.append(o);
    }
    Json::Value resp;
    resp["run_a"]   = a;
    resp["run_b"]   = b;
    resp["entries"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

}  // namespace zion::rest
