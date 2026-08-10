#include "rest_api/controllers/BackupController.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <cstdlib>
#include <ctime>
#include <sstream>

namespace zion::rest {

void BackupController::backup(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    // Retrieve connection string from environment (set at server startup).
    const char* db_url = std::getenv("ZION_DB_URL");
    if (!db_url) {
        Json::Value err; err["error"] = "ZION_DB_URL not set";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k500InternalServerError);
        cb(resp);
        return;
    }

    // Build a timestamped filename.
    std::time_t now = std::time(nullptr);
    char ts_buf[32];
    std::strftime(ts_buf, sizeof(ts_buf), "%Y%m%dT%H%M%SZ", std::gmtime(&now));
    std::string filename = std::string("zion-backup-") + ts_buf + ".dump";

    // Build pg_dump command writing to stdout, captured via popen.
    std::ostringstream cmd;
    cmd << "pg_dump --format=plain " << db_url << " 2>/dev/null";
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        Json::Value err; err["error"] = "pg_dump failed to start";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k500InternalServerError);
        cb(resp);
        return;
    }

    std::string dump;
    dump.reserve(1024 * 1024);  // 1 MB initial reserve
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) {
        dump += buf;
    }
    int rc = pclose(pipe);
    if (rc != 0) {
        Json::Value err; err["error"] = "pg_dump exited with error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k500InternalServerError);
        cb(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeString("application/octet-stream");
    resp->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    resp->setBody(std::move(dump));
    cb(resp);
}

}  // namespace zion::rest
