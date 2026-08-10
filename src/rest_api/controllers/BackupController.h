#pragma once
// rest_api/controllers/BackupController.h — database backup (admin only)
#include <drogon/HttpController.h>

namespace zion::rest {

class BackupController : public drogon::HttpController<BackupController, false> {
public:
    BackupController() = default;

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(BackupController::backup, "/api/v1/backup", drogon::Post);
    METHOD_LIST_END

    // POST /api/v1/backup — triggers pg_dump and returns the dump as a download
    void backup(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);
};

}  // namespace zion::rest
