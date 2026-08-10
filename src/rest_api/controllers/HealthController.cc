#include <fmt/format.h>
#include "rest_api/controllers/HealthController.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include "version.hpp"

namespace zion::rest {

void HealthController::get(const drogon::HttpRequestPtr& /*req*/,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    Json::Value resp;
    resp["status"]  = "ok";
    resp["version"] = ZION_SERVER_VERSION;
    cb(drogon::HttpResponse::newHttpJsonResponse(resp));
}


void HealthController::openapi(const drogon::HttpRequestPtr& /*req*/,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    // Static OpenAPI 3.1 document describing all Zion REST endpoints.
    // This is a trimmed-but-comprehensive spec; extend as new controllers are added.
    static const std::string kSpecTpl = R"openapi({
  "openapi": "3.1.0",
  "info": {
    "title": "Zion IV&V Server API",
    "version": "{}",
    "description": "REST API for the Zion Independent Validation & Verification server."
  },
  "servers": [{ "url": "/api/v1" }],
  "paths": {
    "/health":           { "get": { "summary": "Health check", "operationId": "getHealth", "responses": { "200": { "description": "OK" } } } },
    "/openapi.json":     { "get": { "summary": "OpenAPI spec", "operationId": "getOpenApi", "responses": { "200": { "description": "This document" } } } },
    "/auth/login":       { "post": { "summary": "Login", "operationId": "login", "responses": { "200": { "description": "Session cookie set" } } } },
    "/auth/logout":      { "post": { "summary": "Logout", "operationId": "logout", "responses": { "204": { "description": "Logged out" } } } },
    "/auth/me":          { "get":  { "summary": "Current user", "operationId": "me", "responses": { "200": { "description": "User object" } } } },
    "/auth/password":    { "patch": { "summary": "Change password", "operationId": "changePassword", "responses": { "204": { "description": "Password updated" } } } },
    "/auth/tokens":      { "get":  { "summary": "List API tokens", "operationId": "listTokens", "responses": { "200": { "description": "Token list" } } },
                           "post": { "summary": "Create API token", "operationId": "createToken", "responses": { "201": { "description": "Token created" } } } },
    "/auth/tokens/{id}": { "delete": { "summary": "Revoke token", "operationId": "revokeToken", "responses": { "204": { "description": "Revoked" } } } },
    "/target":           { "get":  { "summary": "Board info", "operationId": "getBoard", "responses": { "200": { "description": "Board object" } } },
                           "put":  { "summary": "Register board", "operationId": "registerBoard", "responses": { "200": { "description": "Updated" } } },
                           "delete": { "summary": "Delete board", "operationId": "deleteBoard", "responses": { "204": { "description": "Deleted" } } } },
    "/runs":             { "get":  { "summary": "List runs", "operationId": "listRuns", "responses": { "200": { "description": "Run list" } } },
                           "post": { "summary": "Create run", "operationId": "createRun", "responses": { "201": { "description": "Run created" } } } },
    "/runs/{id}":        { "get":    { "summary": "Get run", "operationId": "getRun", "responses": { "200": { "description": "Run object" } } },
                           "patch":  { "summary": "Update run", "operationId": "updateRun", "responses": { "200": { "description": "Updated" } } },
                           "delete": { "summary": "Delete run", "operationId": "deleteRun", "responses": { "204": { "description": "Deleted" } } } },
    "/runs/{id}/start":    { "post": { "summary": "Start run", "operationId": "startRun", "responses": { "200": { "description": "Started" } } } },
    "/runs/{id}/cancel":   { "post": { "summary": "Cancel run", "operationId": "cancelRun", "responses": { "200": { "description": "Cancelled" } } } },
    "/runs/{id}/pause":    { "post": { "summary": "Pause run", "operationId": "pauseRun", "responses": { "200": { "description": "Paused" } } } },
    "/runs/{id}/resume":   { "post": { "summary": "Resume run", "operationId": "resumeRun", "responses": { "200": { "description": "Resumed" } } } },
    "/runs/{id}/baseline": { "post": { "summary": "Set as baseline", "operationId": "baselineRun", "responses": { "200": { "description": "Set" } } } },
    "/runs/{id}/rerun":    { "post": { "summary": "Re-run", "operationId": "rerun", "responses": { "201": { "description": "New run created" } } } },
    "/runs/{id}/results":  { "get": { "summary": "List results", "operationId": "listResults", "responses": { "200": { "description": "Result list" } } } },
    "/runs/{id}/events":   { "get": { "summary": "SSE event stream", "operationId": "runEvents", "responses": { "200": { "description": "text/event-stream" } } } },
    "/runs/{id}/export":   { "get": { "summary": "Export run CSV/JSON", "operationId": "exportRun", "responses": { "200": { "description": "Export file" } } } },
    "/runs/{id}/report":   { "get": { "summary": "HTML report", "operationId": "runReport", "responses": { "200": { "description": "text/html" } } } },
    "/logs":               { "get": { "summary": "Global log list", "operationId": "globalLogs", "responses": { "200": { "description": "Log entries" } } } },
    "/logs/search":        { "get": { "summary": "Search logs", "operationId": "searchLogs", "responses": { "200": { "description": "Log entries" } } } },
    "/logs/bookmarks":     { "get":  { "summary": "List bookmarks", "operationId": "listBookmarks", "responses": { "200": { "description": "Bookmarks" } } },
                             "post": { "summary": "Create bookmark", "operationId": "createBookmark", "responses": { "201": { "description": "Created" } } } },
    "/logs/bookmarks/{id}": { "delete": { "summary": "Delete bookmark", "operationId": "deleteBookmark", "responses": { "204": { "description": "Deleted" } } } },
    "/logs/views":          { "get":  { "summary": "List saved views", "operationId": "listViews", "responses": { "200": { "description": "Views" } } },
                              "post": { "summary": "Create view", "operationId": "createView", "responses": { "201": { "description": "Created" } } } },
    "/runs/{id}/logs":     { "get": { "summary": "Run logs", "operationId": "runLogs", "responses": { "200": { "description": "Log entries" } } } },
    "/modules":            { "get": { "summary": "List modules", "operationId": "listModules", "responses": { "200": { "description": "Module list" } } } },
    "/modules/{id}":       { "get": { "summary": "Get module", "operationId": "getModule", "responses": { "200": { "description": "Module object" } } } },
    "/configs":            { "get":  { "summary": "List configs", "operationId": "listConfigs", "responses": { "200": { "description": "Config list" } } },
                             "post": { "summary": "Create config", "operationId": "createConfig", "responses": { "201": { "description": "Created" } } } },
    "/configs/{id}":       { "get":    { "summary": "Get config", "operationId": "getConfig", "responses": { "200": { "description": "Config object" } } },
                             "put":    { "summary": "Update config", "operationId": "updateConfig", "responses": { "200": { "description": "Updated" } } },
                             "delete": { "summary": "Delete config", "operationId": "deleteConfig", "responses": { "204": { "description": "Deleted" } } } },
    "/schedules":          { "get":  { "summary": "List schedules", "operationId": "listSchedules", "responses": { "200": { "description": "Schedules" } } },
                             "post": { "summary": "Create schedule", "operationId": "createSchedule", "responses": { "201": { "description": "Created" } } } },
    "/schedules/{id}":     { "get":    { "summary": "Get schedule", "operationId": "getSchedule", "responses": { "200": { "description": "Schedule" } } },
                             "put":    { "summary": "Update schedule", "operationId": "updateSchedule", "responses": { "200": { "description": "Updated" } } },
                             "delete": { "summary": "Delete schedule", "operationId": "deleteSchedule", "responses": { "204": { "description": "Deleted" } } } },
    "/schedules/{id}/enable":  { "post": { "summary": "Enable schedule", "operationId": "enableSchedule", "responses": { "200": { "description": "Enabled" } } } },
    "/schedules/{id}/disable": { "post": { "summary": "Disable schedule", "operationId": "disableSchedule", "responses": { "200": { "description": "Disabled" } } } },
    "/templates":          { "get":  { "summary": "List templates", "operationId": "listTemplates", "responses": { "200": { "description": "Templates" } } },
                             "post": { "summary": "Create template", "operationId": "createTemplate", "responses": { "201": { "description": "Created" } } } },
    "/templates/{id}":     { "get":    { "summary": "Get template", "operationId": "getTemplate", "responses": { "200": { "description": "Template" } } },
                             "put":    { "summary": "Update template", "operationId": "updateTemplate", "responses": { "200": { "description": "Updated" } } },
                             "delete": { "summary": "Delete template", "operationId": "deleteTemplate", "responses": { "204": { "description": "Deleted" } } } },
    "/templates/{id}/archive":     { "post": { "summary": "Archive template", "operationId": "archiveTemplate", "responses": { "200": { "description": "Archived" } } } },
    "/templates/{id}/instantiate": { "post": { "summary": "Instantiate template into config", "operationId": "instantiateTemplate", "responses": { "201": { "description": "Config created" } } } },
    "/alerts":             { "get":  { "summary": "List alert rules", "operationId": "listAlerts", "responses": { "200": { "description": "Alert rules" } } },
                             "post": { "summary": "Create alert rule", "operationId": "createAlert", "responses": { "201": { "description": "Created" } } } },
    "/alerts/{id}":        { "get":    { "summary": "Get alert rule", "operationId": "getAlert", "responses": { "200": { "description": "Alert rule" } } },
                             "put":    { "summary": "Update alert rule", "operationId": "updateAlert", "responses": { "200": { "description": "Updated" } } },
                             "delete": { "summary": "Delete alert rule", "operationId": "deleteAlert", "responses": { "204": { "description": "Deleted" } } } },
    "/notifications":      { "get": { "summary": "List notifications", "operationId": "listNotifications", "responses": { "200": { "description": "Notifications" } } } },
    "/notifications/{id}/read": { "post": { "summary": "Mark read", "operationId": "markRead", "responses": { "200": { "description": "Marked" } } } },
    "/ota/status":         { "get": { "summary": "OTA status", "operationId": "otaStatus", "responses": { "200": { "description": "OTA state" } } } },
    "/ota/upload":         { "post": { "summary": "Upload OTA image", "operationId": "otaUpload", "responses": { "200": { "description": "Upload initiated" } } } },
    "/ota/apply":          { "post": { "summary": "Apply OTA update", "operationId": "otaApply", "responses": { "200": { "description": "Applied" } } } },
    "/backup":             { "post": { "summary": "Database backup (admin)", "operationId": "backup", "responses": { "200": { "description": "pg_dump stream" } } } }
  }
})openapi";

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeString("application/json");
    resp->setBody(fmt::format(fmt::runtime(kSpecTpl), ZION_SERVER_VERSION));
    cb(resp);
}

}  // namespace zion::rest
