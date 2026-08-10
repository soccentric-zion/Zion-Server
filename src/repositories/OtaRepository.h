#pragma once
// repositories/OtaRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct OtaUpgrade {
    std::string id;
    std::string artifact_path;
    std::string artifact_hash;
    int64_t     artifact_size = 0;
    std::string from_version;
    std::string to_version;
    std::string status;
    int         progress = 0;
    std::string started_at;
    std::string ended_at;
    std::string error_message;
    std::string rolled_back_from;
    std::string created_at;
    std::string created_by;
};

class OtaRepository {
public:
    OtaUpgrade              create(const OtaUpgrade& u);
    std::optional<OtaUpgrade> find_by_id(const std::string& id);
    std::vector<OtaUpgrade> list();
    void                    update_status(const std::string& id, const std::string& status,
                                          int progress = -1,
                                          const std::string& error = "");
    void                    set_ended(const std::string& id, const std::string& status,
                                      const std::string& error = "");
};

}  // namespace zion
