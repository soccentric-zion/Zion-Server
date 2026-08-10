#pragma once
// services/OtaService.h
#include "config/Settings.h"
#include "repositories/OtaRepository.h"
#include "repositories/AuditRepository.h"
#include <string>
#include <vector>

namespace zion {

class OtaService {
public:
    explicit OtaService(const OtaSettings& cfg);

    // Accept an uploaded artifact. Returns artifact id.
    OtaUpgrade accept_artifact(const std::string& filename,
                                const std::string& tmp_path,
                                const std::string& actor_user_id);

    // Trigger an upgrade to the board.
    OtaUpgrade trigger_upgrade(const std::string& artifact_id,
                                const std::string& actor_user_id);

    OtaUpgrade get(const std::string& id);
    std::vector<OtaUpgrade> list();

    // Update progress (called by OtaOrchestrator).
    void update_progress(const std::string& id, int progress, const std::string& status);

    // Finalize.
    void finalize(const std::string& id, const std::string& status, const std::string& error = "");

    // Rollback: creates a new OtaUpgrade with rolled_back_from set.
    OtaUpgrade rollback(const std::string& upgrade_id, const std::string& actor_user_id);

private:
    OtaSettings    cfg_;
    OtaRepository  repo_;
    AuditRepository audit_repo_;

    std::string permanent_path(const std::string& artifact_id,
                                const std::string& filename) const;
};

}  // namespace zion
