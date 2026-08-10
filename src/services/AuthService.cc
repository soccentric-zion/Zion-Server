#include "services/AuthService.h"
#include "util/Hash.h"
#include "util/Errors.h"
#include "util/Uuid.h"

#include <spdlog/spdlog.h>

#include <stdexcept>

namespace zion {

AuthService::AuthService(const AuthSettings& cfg) : cfg_(cfg) {}

std::optional<LoginResult> AuthService::login(const std::string& username,
                                               const std::string& password,
                                               const std::string& remote_ip) {
    auto user_opt = user_repo_.find_by_username(username);
    if (!user_opt) {
        spdlog::info("auth: login failed (unknown user) username={} ip={}",
                     username, remote_ip);
        return std::nullopt;
    }

    if (!util::argon2id_verify(password, user_opt->password_hash)) {
        spdlog::info("auth: login failed (bad password) username={} ip={}",
                     username, remote_ip);
        audit_repo_.log("", "auth.login.failed", "user", user_opt->id,
                        R"({"ip":")" + remote_ip + "\"}");
        return std::nullopt;
    }

    // Generate a session token (random hex, stored in signed cookie by the HTTP layer)
    std::string token = util::random_token(32);
    audit_repo_.log(user_opt->id, "auth.login", "user", user_opt->id,
                    R"({"ip":")" + remote_ip + "\"}");
    spdlog::info("auth: login ok username={}", username);
    return LoginResult{*user_opt, token};
}

std::optional<std::string> AuthService::validate_session(const std::string& /*token*/) {
    // NOTE: Session token validation is handled at the HTTP filter layer using
    // Drogon's session mechanism.  This method is a hook for custom session
    // stores; in the default configuration it is not used.
    return std::nullopt;
}

std::pair<ApiToken, std::string> AuthService::create_api_token(
    const std::string& user_id,
    const std::string& name) {
    std::string plaintext = util::random_token(32);
    std::string hash      = util::sha256_hex(plaintext);
    auto token = token_repo_.create(user_id, name, hash);
    audit_repo_.log(user_id, "token.created", "api_token", token.id, "{}");
    return {token, plaintext};
}

std::optional<std::string> AuthService::authenticate_token(
    const std::string& plaintext_token) {
    std::string hash = util::sha256_hex(plaintext_token);
    auto token_opt   = token_repo_.find_by_hash(hash);
    if (!token_opt) return std::nullopt;
    token_repo_.touch(token_opt->id);
    return token_opt->user_id;
}

void AuthService::change_password(const std::string& user_id,
                                   const std::string& old_password,
                                   const std::string& new_password) {
    auto user_opt = user_repo_.find_by_id(user_id);
    if (!user_opt) throw AuthError("user not found");

    if (!util::argon2id_verify(old_password, user_opt->password_hash)) {
        throw AuthError("incorrect current password");
    }

    util::Argon2Params p{cfg_.argon2_mem_kib, cfg_.argon2_iter, cfg_.argon2_parallel};
    std::string new_hash = util::argon2id_hash(new_password, p);
    user_repo_.update_password(user_id, new_hash);
    audit_repo_.log(user_id, "auth.password_changed", "user", user_id, "{}");
}

User AuthService::ensure_admin(const std::string& username,
                                const std::string& password) {
    auto existing = user_repo_.find_by_username(username);
    if (existing) return *existing;

    util::Argon2Params p{cfg_.argon2_mem_kib, cfg_.argon2_iter, cfg_.argon2_parallel};
    std::string hash = util::argon2id_hash(password, p);
    auto user = user_repo_.create(username, hash);
    spdlog::info("auth: created admin user '{}'", username);
    return user;
}

std::vector<ApiToken> AuthService::list_tokens(const std::string& user_id) {
    return token_repo_.list_for_user(user_id);
}

void AuthService::revoke_token(const std::string& token_id,
                                const std::string& actor_user_id) {
    token_repo_.revoke(token_id);
    audit_repo_.log(actor_user_id, "token.revoked", "api_token", token_id, "{}");
}

std::optional<User> AuthService::get_user(const std::string& user_id) {
    return user_repo_.find_by_id(user_id);
}

}  // namespace zion
