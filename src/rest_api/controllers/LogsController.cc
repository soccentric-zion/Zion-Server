#include "rest_api/controllers/LogsController.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <queue>

namespace zion::rest {

LogsController::LogsController(LogService& svc) : svc_(svc) {}

void LogsController::list(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                           std::string run_id) {
    LogFilter f;
    f.run_id = run_id;
    if (auto p = req->getParameter("q"); !p.empty()) f.query = p;
    if (auto p = req->getParameter("level"); !p.empty()) f.severity = p;
    if (auto p = req->getParameter("limit"); !p.empty()) f.limit = std::stoi(p);
    if (auto p = req->getParameter("offset"); !p.empty()) f.offset = std::stoi(p);

    auto entries = svc_.list(f);
    Json::Value arr{Json::arrayValue};
    for (const auto& e : entries) {
        Json::Value v;
        v["id"]        = e.id;
        v["run_id"]    = e.run_id;
        v["level"]     = e.severity;
        v["message"]   = e.message;
        v["timestamp"] = e.timestamp;
        arr.append(v);
    }
    Json::Value resp;
    resp["logs"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

static Json::Value log_entry_to_json(const LogEntry& e) {
    Json::Value v;
    v["id"]        = e.id;
    v["run_id"]    = e.run_id;
    v["source"]    = e.source;
    v["level"]     = e.severity;
    v["severity"]  = e.severity;
    v["message"]   = e.message;
    v["timestamp"] = e.timestamp;
    return v;
}

// SSE context for log tail streaming.
namespace {
struct LogSseCtx {
    std::mutex              mtx;
    std::condition_variable cv;
    std::queue<std::string> msgs;
    bool                    done{false};
};
}  // namespace

void LogsController::tail(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                           std::string run_id) {
    // Filter context for this tail subscription (run_id may be empty = all logs).
    auto ctx = std::make_shared<LogSseCtx>();

    // Push a connected event immediately.
    {
        std::lock_guard lk(ctx->mtx);
        std::string connected = "event: connected\ndata: {\"status\":\"connected\"}\n\n";
        ctx->msgs.push(std::move(connected));
    }
    ctx->cv.notify_one();

    // Subscribe to broadcast log entries.
    Json::StreamWriterBuilder fwb;
    fwb["indentation"] = "";
    uint64_t sub_id = svc_.subscribe([ctx, run_id, fwb](const LogEntry& e) {
        // If a run_id filter is set, skip entries not matching it.
        if (!run_id.empty() && e.run_id != run_id) return;
        std::ostringstream ss;
        std::unique_ptr<Json::StreamWriter>(fwb.newStreamWriter())->write(log_entry_to_json(e), &ss);
        std::string line = "event: log\ndata: " + ss.str() + "\n\n";
        {
            std::lock_guard lk(ctx->mtx);
            ctx->msgs.push(std::move(line));
        }
        ctx->cv.notify_one();
    });

    auto resp = drogon::HttpResponse::newStreamResponse(
        [ctx, sub_id_captured = sub_id, &svc = svc_](char* buf, std::size_t len) mutable -> std::size_t {
            std::unique_lock lk(ctx->mtx);
            ctx->cv.wait_for(lk, std::chrono::seconds(20), [&]{
                return !ctx->msgs.empty() || ctx->done;
            });
            if (ctx->msgs.empty()) {
                if (ctx->done) {
                    svc.unsubscribe(sub_id_captured);
                    return 0;
                }
                static const char ka[] = ": keepalive\n\n";
                static const std::size_t ka_len = sizeof(ka) - 1;
                std::size_t n = std::min(ka_len, len);
                std::memcpy(buf, ka, n);
                return n;
            }
            std::string msg = std::move(ctx->msgs.front());
            ctx->msgs.pop();
            lk.unlock();
            std::size_t n = std::min(msg.size(), len);
            std::memcpy(buf, msg.data(), n);
            return n;
        },
        /*attachmentFileName=*/"",
        drogon::CT_CUSTOM,
        "text/event-stream");
    resp->addHeader("Cache-Control", "no-cache");
    if (!run_id.empty()) resp->addHeader("X-Zion-Run-Id", run_id);
    cb(resp);
}

void LogsController::global_list(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    LogFilter f;
    if (auto p = req->getParameter("source"); !p.empty())  f.source   = p;
    if (auto p = req->getParameter("level");  !p.empty())  f.severity = p;
    if (auto p = req->getParameter("run_id"); !p.empty())  f.run_id   = p;
    if (auto p = req->getParameter("query");  !p.empty())  f.query    = p;
    if (auto p = req->getParameter("after");  !p.empty())  f.from_ts  = p;
    if (auto p = req->getParameter("limit");  !p.empty())  f.limit    = std::stoi(p);
    if (auto p = req->getParameter("offset"); !p.empty())  f.offset   = std::stoi(p);

    auto entries = svc_.list(f);
    Json::Value arr{Json::arrayValue};
    for (const auto& e : entries) arr.append(log_entry_to_json(e));
    Json::Value resp;
    resp["logs"]  = arr;
    resp["total"] = static_cast<int>(entries.size());
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void LogsController::search(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    LogFilter f;
    if (auto p = req->getParameter("q");      !p.empty()) f.query  = p;
    if (auto p = req->getParameter("run_id"); !p.empty()) f.run_id = p;
    if (auto p = req->getParameter("limit");  !p.empty()) f.limit  = std::stoi(p);
    f.limit = std::min(f.limit, 500);

    auto entries = svc_.list(f);
    Json::Value arr{Json::arrayValue};
    for (const auto& e : entries) arr.append(log_entry_to_json(e));
    Json::Value resp;
    resp["logs"]  = arr;
    resp["total"] = static_cast<int>(entries.size());
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void LogsController::list_bookmarks(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto items = svc_.list_bookmarks(user_id);
    Json::Value arr{Json::arrayValue};
    for (const auto& b : items) {
        Json::Value v;
        v["id"]         = b.id;
        v["log_id"]     = b.log_id;
        v["note"]       = b.note;
        v["created_at"] = b.created_at;
        arr.append(v);
    }
    Json::Value resp;
    resp["bookmarks"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void LogsController::create_bookmark(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto body = req->getJsonObject();
    std::string log_id = body ? (*body)["log_id"].asString() : "";
    std::string note   = body ? (*body)["note"].asString()   : "";
    auto b = svc_.create_bookmark(user_id, log_id, note);
    Json::Value v;
    v["id"]         = b.id;
    v["log_id"]     = b.log_id;
    v["note"]       = b.note;
    v["created_at"] = b.created_at;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(v);
    resp->setStatusCode(drogon::k201Created);
    cb(resp);
}

void LogsController::delete_bookmark(const drogon::HttpRequestPtr& /*req*/,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                      std::string id) {
    svc_.remove_bookmark(id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void LogsController::list_views(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto items = svc_.list_views(user_id);
    Json::Value arr{Json::arrayValue};
    for (const auto& v : items) {
        Json::Value o;
        o["id"]           = v.id;
        o["name"]         = v.name;
        o["filters_json"] = v.filters_json;
        o["created_at"]   = v.created_at;
        arr.append(o);
    }
    Json::Value resp;
    resp["views"] = arr;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void LogsController::create_view(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    auto body  = req->getJsonObject();
    std::string name         = body ? (*body)["name"].asString()         : "";
    std::string filters_json = body ? (*body)["filters_json"].asString() : "{}";
    auto sv = svc_.create_view(user_id, name, filters_json);
    Json::Value o;
    o["id"]           = sv.id;
    o["name"]         = sv.name;
    o["filters_json"] = sv.filters_json;
    o["created_at"]   = sv.created_at;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(o);
    resp->setStatusCode(drogon::k201Created);
    cb(resp);
}

}  // namespace zion::rest
