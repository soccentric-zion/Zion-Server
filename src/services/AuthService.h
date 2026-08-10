#pragma once
// services/AuthService.h — authentication and token management.
#include "config/Settings.h"
#include "repositories/UserRepository.h"
#include "repositories/ApiTokenRepository.h"
#include "repositories/AuditRepository.h"

#include <optional>
#include <string>

namespace zion {

struct LoginResult {
    User        user;
    std::string session_token;  // opaque random token stored in signed cookie
};

class AuthService {
public:
    explicit AuthService(const AuthSettings& cfg);

    // Login with username/password. Returns user on success, nullopt on bad credentials.
    std::optional<LoginResult> login(const std::string& username,
                                     const std::string& password,
                                     const std::string& remote_ip);

    // Validate a session token. Returns user_id on success.
    std::optional<std::string> validate_session(const std::string& token);

    // Create a new API token.  Returns {id, plaintext_token}.
    std::pair<ApiToken, std::string> create_api_token(const std::string& user_id,
                                                       const std::string& name);

    // Authenticate via API token. Returns user_id on success.
    std::optional<std::string> authenticate_token(const std::string& plaintext_token);

    // Change password.  Throws AuthError if old_password is wrong.
    void change_password(const std::string& user_id,
                         const std::string& old_password,
                         const std::string& new_password);

    // Create the initial admin user (only if no users exist yet).
    User ensure_admin(const std::string& username, const std::string& password);

    // List tokens for user.
    std::vector<ApiToken> list_tokens(const std::string& user_id);

    // Revoke a token.
    void revoke_token(const std::string& token_id, const std::string& actor_user_id);

    // Get user by id.
    std::optional<User> get_user(const std::string& user_id);

private:
    AuthSettings         cfg_;
    UserRepository       user_repo_;
    ApiTokenRepository   token_repo_;
    AuditRepository      audit_repo_;
};

}  // namespace zion
