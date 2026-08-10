#pragma once
// repositories/AuditRepository.h
#include <string>
#include <vector>

namespace zion {

struct AuditEntry {
    std::string id;
    std::string user_id;
    std::string action;
    std::string target_type;
    std::string target_id;
    std::string metadata_json;
    std::string created_at;
};

struct AuditFilter {
    std::string user_id;
    std::string action;
    std::string target_type;
    std::string from_ts;
    std::string to_ts;
    int limit  = 100;
    int offset = 0;
};

class AuditRepository {
public:
    void log(const std::string& user_id,
             const std::string& action,
             const std::string& target_type,
             const std::string& target_id,
             const std::string& metadata_json = "{}");

    std::vector<AuditEntry> list(const AuditFilter& f);
};

}  // namespace zion
