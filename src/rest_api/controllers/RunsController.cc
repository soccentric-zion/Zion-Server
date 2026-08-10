#include "rest_api/controllers/RunsController.h"
#include "repositories/ResultRepository.h"
#include "util/Errors.h"
#include "util/ResultConvert.h"
#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>
#include <json/json.h>
#include <map>
#include <sstream>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <cstring>

namespace zion::rest {

// Canonical mode mapping (REST wire string u2192 canonical form)
// Dashboard sends: "single_shot", "long_run", "monitoring", "event"
// Mapping to proto ExecutionMode: single_shot=1, long_run=2, monitoring=3, event=4
static std::string normalize_mode(const std::string& raw) {
    if (raw == "single_shot" || raw == "1") return "single_shot";
    if (raw == "long_run"    || raw == "2") return "long_run";
    if (raw == "monitoring"  || raw == "3") return "monitoring";
    if (raw == "event"       || raw == "4") return "event";
    return raw.empty() ? "single_shot" : raw;
}


// ── Helpers ───────────────────────────────────────────────────────────────────

static Json::Value run_to_json(const Run& r) {
    Json::Value v;
    v["id"]               = r.id;
    v["name"]             = r.name;
    v["description"]      = r.description;
    v["mode"]             = r.mode;
    v["status"]           = r.status;
    v["baseline_flag"]    = r.baseline_flag;
    v["notes"]            = r.notes;
    v["error_message"]    = r.error_message;
    v["created_at"]       = r.created_at;
    v["created_by"]       = r.created_by;
    v["started_at"]       = r.started_at;
    v["ended_at"]         = r.ended_at;
    if (!r.tags_json.empty() && r.tags_json != "null") {
        Json::CharReaderBuilder rb;
        Json::Value tags;
        std::string errs;
        std::istringstream ss(r.tags_json);
        if (Json::parseFromStream(rb, ss, &tags, &errs)) v["tags"] = tags;
        else v["tags"] = Json::objectValue;
    } else {
        v["tags"] = Json::objectValue;
    }
    return v;
}

// Build the canonical REST result DTO.
static Json::Value result_to_json(const Result& r) {
    Json::Value o;
    o["id"]               = r.id;
    o["run_id"]           = r.run_id;
    o["module_id"]        = r.module_id;
    o["module_version"]   = r.module_version;
    o["test_id"]          = r.test_id;
    o["name"]             = r.test_name;
    o["sequence"]         = static_cast<Json::Int64>(r.sequence);
    o["timestamp"]        = r.timestamp;
    o["duration_ns"]      = static_cast<Json::Int64>(r.duration_ns);
    o["status"]           = util::status_to_string(r.status);
    o["severity"]         = util::severity_to_string(r.severity);
    o["error_message"]    = r.error_message;
    o["mode"]             = r.mode;

    // JSONB typed maps — parse from stored string or emit empty object.
    auto parse_or_empty = [](const std::string& s) {
        return util::parse_typed_map_json(s);
    };
    o["reason"]     = parse_or_empty(r.reason_json);
    o["metrics"]    = parse_or_empty(r.metrics_json);
    o["monitoring"] = parse_or_empty(r.monitoring_json);
    o["event"]      = parse_or_empty(r.event_json);

    // Subtests (in-memory, populated when caller uses get_subtests).
    Json::Value subtests_arr{Json::arrayValue};
    for (const auto& sub : r.subtests) subtests_arr.append(result_to_json(sub));
    o["subtests"] = subtests_arr;

    o["payload_type_url"] = r.payload_type_url;
    // payload bytes as base64.
    if (!r.payload_bytes.empty()) {
        o["payload_b64"] = drogon::utils::base64Encode(
            reinterpret_cast<const unsigned char*>(r.payload_bytes.data()),
            static_cast<unsigned int>(r.payload_bytes.size()));
    } else {
        o["payload_b64"] = "";
    }
    return o;
}

// Escape a CSV field (quote if contains comma/quote/newline).
static std::string csv_field(const std::string& s) {
    if (s.find_first_of(",\"\n\r") == std::string::npos) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

// Flatten a result row into CSV columns per the frozen column order:
// run_id, module_id, test_id, name, sequence, timestamp, duration_ns,
// status, severity, error_message, reason_json, metrics_json, monitoring_json, event_json
// Subtests add a leading parent_result_id column.
static void append_csv_rows(std::ostringstream& out, const Result& r,
                              const std::string& parent_id = "") {
    if (!parent_id.empty()) {
        out << csv_field(parent_id) << ",";
    }
    out << csv_field(r.run_id)       << ","
        << csv_field(r.module_id)    << ","
        << csv_field(r.test_id)      << ","
        << csv_field(r.test_name)    << ","
        << r.sequence                << ","
        << csv_field(r.timestamp)    << ","
        << r.duration_ns             << ","
        << util::status_to_string(r.status)   << ","
        << util::severity_to_string(r.severity) << ","
        << csv_field(r.error_message)           << ","
        << csv_field(r.reason_json)             << ","
        << csv_field(r.metrics_json)            << ","
        << csv_field(r.monitoring_json)         << ","
        << csv_field(r.event_json)              << "\n";

    for (const auto& sub : r.subtests) {
        append_csv_rows(out, sub, r.id);
    }
}

// ── Controller methods ────────────────────────────────────────────────────────

RunsController::RunsController(RunService& svc) : svc_(svc) {}

void RunsController::list(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    RunFilter f;
    f.status = req->getParameter("status");
    f.mode   = req->getParameter("mode");
    auto lim = req->getParameter("limit");
    auto off = req->getParameter("offset");
    if (!lim.empty()) f.limit  = std::stoi(lim);
    if (!off.empty()) f.offset = std::stoi(off);
    auto runs = svc_.list_runs(f);
    Json::Value arr{Json::arrayValue};
    for (const auto& r : runs) arr.append(run_to_json(r));
    Json::Value resp;
    resp["runs"]  = arr;
    resp["total"] = static_cast<int>(runs.size());
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void RunsController::create(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto body = req->getJsonObject();
    std::string config_id = body ? (*body)["config_id"].asString() : "";
    std::string mode      = normalize_mode(body ? (*body)["mode"].asString() : "single_shot");
    std::string user_id   = req->getAttributes()->get<std::string>("user_id");
    auto run = svc_.start_run(config_id, mode, mode, user_id);
    cb(drogon::HttpResponse::newHttpJsonResponse(run_to_json(run)));
}

void RunsController::get(const drogon::HttpRequestPtr& /*req*/,
                          std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                          std::string id) {
    try {
        auto run = svc_.get_run(id);
        cb(drogon::HttpResponse::newHttpJsonResponse(run_to_json(run)));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void RunsController::cancel(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.cancel_run(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void RunsController::pause(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                            std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.pause_run(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void RunsController::resume(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    svc_.resume_run(id, user_id);
    Json::Value resp; resp["ok"] = true;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

void RunsController::results(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              std::string id) {
    ResultFilter f;
    f.run_id          = id;
    f.top_level_only  = true;  // subtests fetched separately via /subtests endpoint.
    if (auto v = req->getOptionalParameter<std::string>("module_id")) f.module_id = *v;
    if (auto v = req->getOptionalParameter<std::string>("test_id"))   f.test_id   = *v;
    if (auto v = req->getOptionalParameter<std::string>("since"))     f.since     = *v;
    if (auto v = req->getOptionalParameter<std::string>("until"))     f.until     = *v;
    if (auto v = req->getOptionalParameter<int>("limit"))             f.limit     = *v;
    if (auto v = req->getOptionalParameter<int>("offset"))            f.offset    = *v;

    // Parse status filter (comma-separated uppercase strings e.g. "PASS,FAIL").
    if (auto v = req->getOptionalParameter<std::string>("status")) {
        std::istringstream ss(*v);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            int16_t s = util::string_to_status(tok);
            if (s > 0) f.statuses.push_back(s);
        }
    }
    // Parse severity filter.
    if (auto v = req->getOptionalParameter<std::string>("severity")) {
        std::istringstream ss(*v);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            int16_t sev = util::string_to_severity(tok);
            if (sev > 0) f.severities.push_back(sev);
        }
    }

    auto rows = svc_.get_results(f);

    Json::Value arr(Json::arrayValue);
    for (const auto& r : rows) arr.append(result_to_json(r));

    Json::Value resp;
    resp["results"] = arr;
    resp["total"]   = static_cast<int>(rows.size());
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}

// SSE context for live run event streaming — shared between subscribe callback
// and the Drogon stream-response callback.
namespace {
struct RunSseCtx {
    std::mutex              mtx;
    std::condition_variable cv;
    std::queue<std::string> msgs;    // ready-to-send SSE lines
    bool                    done{false};
};
}  // namespace

void RunsController::events(const drogon::HttpRequestPtr& /*req*/,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             std::string id) {
    // Emit initial run-state event.
    std::string initial;
    try {
        auto run = svc_.get_run(id);
        initial  = "event: state\ndata: {\"type\":\"state\",\"run_id\":\"" + id
                 + "\",\"status\":\"" + run.status + "\"}\n\n";
    } catch (...) {
        initial  = "event: error\ndata: {\"type\":\"error\","
                   "\"message\":\"run not found\"}\n\n";
    }

    auto ctx = std::make_shared<RunSseCtx>();
    {
        std::lock_guard lk(ctx->mtx);
        ctx->msgs.push(std::move(initial));
    }
    ctx->cv.notify_one();

    // Subscribe to live results for this run.
    Json::StreamWriterBuilder fwb;
    fwb["indentation"] = "";
    uint64_t sub_id = svc_.subscribe_results(id, [ctx, fwb](const Result& r) {
        std::ostringstream ss;
        std::unique_ptr<Json::StreamWriter>(fwb.newStreamWriter())->write(result_to_json(r), &ss);
        std::string line = "event: result\ndata: " + ss.str() + "\n\n";
        {
            std::lock_guard lk(ctx->mtx);
            ctx->msgs.push(std::move(line));
        }
        ctx->cv.notify_one();
    });

    auto resp = drogon::HttpResponse::newStreamResponse(
        [ctx, sub_id_captured = sub_id, &svc = svc_](char* buf, std::size_t len) mutable -> std::size_t {
            std::unique_lock lk(ctx->mtx);
            // Wait up to 20 s for new data; send keepalive comment on timeout.
            ctx->cv.wait_for(lk, std::chrono::seconds(20), [&]{
                return !ctx->msgs.empty() || ctx->done;
            });
            if (ctx->msgs.empty()) {
                if (ctx->done) {
                    svc.unsubscribe_results(sub_id_captured);
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
    resp->addHeader("X-Zion-Run-Id", id);
    cb(resp);
}

void RunsController::remove(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    try {
        svc_.delete_run(id, user_id);
        Json::Value resp; resp["ok"] = true;
        cb(drogon::HttpResponse::newHttpJsonResponse(resp));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void RunsController::baseline(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                               std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    try {
        svc_.set_baseline(id, user_id);
        Json::Value resp; resp["ok"] = true;
        cb(drogon::HttpResponse::newHttpJsonResponse(resp));
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void RunsController::rerun(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                            std::string id) {
    std::string user_id = req->getAttributes()->get<std::string>("user_id");
    try {
        auto new_run = svc_.rerun(id, user_id);
        auto resp = drogon::HttpResponse::newHttpJsonResponse(run_to_json(new_run));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

void RunsController::export_run(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                 std::string id) {
    std::string fmt = req->getParameter("format");
    if (fmt.empty()) fmt = "json";

    ResultFilter f;
    f.run_id         = id;
    f.top_level_only = false;  // Include subtests in exports.
    f.limit          = 10000;
    auto rows = svc_.get_results(f);

    auto resp = drogon::HttpResponse::newHttpResponse();
    if (fmt == "csv") {
        std::ostringstream csv;
        // Header — parent_result_id is prepended only for subtest rows; top-level have it empty.
        csv << "parent_result_id,"
               "run_id,module_id,test_id,name,sequence,timestamp,duration_ns,"
               "status,severity,error_message,reason_json,metrics_json,monitoring_json,event_json\n";
        for (const auto& r : rows) {
            append_csv_rows(csv, r);
        }
        resp->setContentTypeString("text/csv");
        resp->addHeader("Content-Disposition", "attachment; filename=\"run-" + id + ".csv\"");
        resp->setBody(csv.str());
    } else {
        // NDJSON export.
        std::ostringstream ndjson;
        for (const auto& r : rows) {
            Json::FastWriter fw;
            std::string line = fw.write(result_to_json(r));
            if (!line.empty() && line.back() == '\n') line.pop_back();
            ndjson << line << "\n";
        }
        resp->setContentTypeString("application/x-ndjson");
        resp->addHeader("Content-Disposition", "attachment; filename=\"run-" + id + ".ndjson\"");
        resp->setBody(ndjson.str());
    }
    cb(resp);
}

void RunsController::report(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                             std::string id) {
    std::string fmt = req->getParameter("format");
    if (fmt.empty()) fmt = "html";

    try {
        auto run  = svc_.get_run(id);
        ResultFilter f;
        f.run_id         = id;
        f.top_level_only = true;
        f.limit          = 10000;
        auto rows = svc_.get_results(f);

        // Build pass/fail counts per module using numeric status codes.
        // PASS=1, all others count as failures for the report summary.
        std::map<std::string, int> pass_count, fail_count;
        for (const auto& r : rows) {
            if (r.status == 1) pass_count[r.module_id]++;
            else               fail_count[r.module_id]++;
        }

        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
             << "<title>Zion Report — " << run.name << "</title>"
             << "<style>body{font-family:sans-serif;margin:2rem}table{border-collapse:collapse;width:100%}"
             << "th,td{border:1px solid #ccc;padding:.5rem .75rem;text-align:left}"
             << "th{background:#f5f5f5}.pass{color:#16a34a}.fail{color:#dc2626}</style></head><body>"
             << "<h1>Zion Run Report</h1>"
             << "<table><tr><th>Field</th><th>Value</th></tr>"
             << "<tr><td>Run ID</td><td>" << run.id << "</td></tr>"
             << "<tr><td>Name</td><td>" << run.name << "</td></tr>"
             << "<tr><td>Mode</td><td>" << run.mode << "</td></tr>"
             << "<tr><td>Status</td><td>" << run.status << "</td></tr>"
             << "<tr><td>Started</td><td>" << run.started_at << "</td></tr>"
             << "<tr><td>Ended</td><td>" << run.ended_at << "</td></tr>"
             << "</table><h2>Module Results</h2>"
             << "<table><tr><th>Module</th><th>Pass</th><th>Fail</th></tr>";
        for (const auto& [mod, pc] : pass_count) {
            int fc = fail_count.count(mod) ? fail_count.at(mod) : 0;
            html << "<tr><td>" << mod << "</td>"
                 << "<td class=\"pass\">" << pc << "</td>"
                 << "<td class=\"" << (fc > 0 ? "fail" : "pass") << "\">" << fc << "</td></tr>";
        }
        for (const auto& [mod, fc] : fail_count) {
            if (!pass_count.count(mod)) {
                html << "<tr><td>" << mod << "</td><td class=\"pass\">0</td>"
                     << "<td class=\"fail\">" << fc << "</td></tr>";
            }
        }
        html << "</table></body></html>";

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setContentTypeString("text/html; charset=utf-8");
        resp->addHeader("Content-Disposition",
                        "attachment; filename=\"report-" + id + ".html\"");
        resp->setBody(html.str());
        cb(resp);
    } catch (const NotFoundError& e) {
        Json::Value err; err["error"] = e.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        cb(resp);
    }
}

}  // namespace zion::rest
