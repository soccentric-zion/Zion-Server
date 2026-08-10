#pragma once
// repositories/ApiTokenRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct ApiToken {
    std::string id;
    std::string user_id;
    std::string name;
    std::string token_hash;
    std::string last_used_at;
    std::string created_at;
    std::string revoked_at;
};

class ApiTokenRepository {
public:
    // Create a new token record (token_hash is SHA-256 of the plaintext).
    ApiToken create(const std::string& user_id,
                    const std::string& name,
                    const std::string& token_hash);

    // Find by token_hash (used during authentication).
    std::optional<ApiToken> find_by_hash(const std::string& token_hash);

    // List all non-revoked tokens for a user.
    std::vector<ApiToken> list_for_user(const std::string& user_id);

    // Revoke a token by id.
    void revoke(const std::string& id);

    // Update last_used_at to now.
    void touch(const std::string& id);
};

}  // namespace zion
