#include "services/OtaService.h"
#include "util/Hash.h"
#include "util/Uuid.h"
#include "util/Errors.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace zion {

namespace fs = std::filesystem;

OtaService::OtaService(const OtaSettings& cfg) : cfg_(cfg) {
    fs::create_directories(cfg_.storage_path);
}

OtaUpgrade OtaService::accept_artifact(const std::string& filename,
                                        const std::string& tmp_path,
                                        const std::string& actor_user_id) {
    // Compute hash of the uploaded file.
    std::string hash = util::sha256_file_hex(tmp_path);

    // Move to permanent storage.
    std::string artifact_id = util::generate_uuid();
    std::string dest = permanent_path(artifact_id, filename);
    fs::create_directories(fs::path(dest).parent_path());
    fs::rename(tmp_path, dest);

    int64_t size = static_cast<int64_t>(fs::file_size(dest));

    OtaUpgrade u;
    u.artifact_path = dest;
    u.artifact_hash = hash;
    u.artifact_size = size;
    u.status        = "uploaded";
    u.created_by    = actor_user_id;

    auto saved = repo_.create(u);
    audit_repo_.log(actor_user_id, "ota.artifact.uploaded", "ota_upgrade", saved.id,
                    R"({"filename":")" + filename + "\"}");
    spdlog::info("ota: artifact accepted id={} hash={} size={}", saved.id, hash, size);
    return saved;
}

OtaUpgrade OtaService::trigger_upgrade(const std::string& artifact_id,
                                        const std::string& actor_user_id) {
    auto opt = repo_.find_by_id(artifact_id);
    if (!opt) throw NotFoundError("ota artifact " + artifact_id);
    if (opt->status != "uploaded") {
        throw ValidationError("artifact is not in 'uploaded' state");
    }

    repo_.update_status(artifact_id, "pending", 0);
    audit_repo_.log(actor_user_id, "ota.upgrade.triggered", "ota_upgrade", artifact_id, "{}");
    spdlog::info("ota: upgrade triggered artifact_id={}", artifact_id);

    // OtaOrchestrator will pick this up via polling.
    return *repo_.find_by_id(artifact_id);
}

OtaUpgrade OtaService::get(const std::string& id) {
    auto opt = repo_.find_by_id(id);
    if (!opt) throw NotFoundError("ota upgrade " + id);
    return *opt;
}

std::vector<OtaUpgrade> OtaService::list() {
    return repo_.list();
}

void OtaService::update_progress(const std::string& id, int progress,
                                  const std::string& status) {
    repo_.update_status(id, status, progress);
}

void OtaService::finalize(const std::string& id, const std::string& status,
                           const std::string& error) {
    repo_.set_ended(id, status, error);
    spdlog::info("ota: finalized id={} status={}", id, status);
}

OtaUpgrade OtaService::rollback(const std::string& upgrade_id,
                                  const std::string& actor_user_id) {
    auto orig = get(upgrade_id);
    OtaUpgrade rb;
    rb.artifact_path    = orig.artifact_path;
    rb.artifact_hash    = orig.artifact_hash;
    rb.artifact_size    = orig.artifact_size;
    rb.from_version     = orig.to_version;
    rb.to_version       = orig.from_version;
    rb.status           = "pending";
    rb.rolled_back_from = upgrade_id;
    rb.created_by       = actor_user_id;
    auto saved = repo_.create(rb);
    audit_repo_.log(actor_user_id, "ota.rollback.triggered", "ota_upgrade", saved.id,
                    R"({"rolled_back_from":")" + upgrade_id + "\"}");
    return saved;
}

std::string OtaService::permanent_path(const std::string& artifact_id,
                                        const std::string& filename) const {
    return cfg_.storage_path + "/" + artifact_id + "/" + filename;
}

}  // namespace zion
