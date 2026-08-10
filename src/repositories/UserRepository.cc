#include "repositories/UserRepository.h"
#include "db/DbClient.h"
#include "util/Uuid.h"
#include "util/Errors.h"

#include <drogon/orm/DbClient.h>
#include <spdlog/spdlog.h>

namespace zion {

User UserRepository::create(const std::string& username,
                            const std::string& password_hash) {
    auto db = db::get_db_client();
    try {
        auto rows = db->execSqlSync(
            "INSERT INTO users (id, username, password_hash)"
            " VALUES (gen_random_uuid(), $1, $2)"
            " RETURNING id,  username,  password_hash,  created_at,  updated_at", 
            username.c_str(),  password_hash.c_str());
        if (rows.empty()) throw std::runtime_error("UserRepository::create: no rows returned");
        const auto& r = rows[0];
        return User{
            r["id"].as<std::string>(),
            r["username"].as<std::string>(),
            r["password_hash"].as<std::string>(),
            r["created_at"].as<std::string>(),
            r["updated_at"].as<std::string>()
        };
    } catch (const drogon::orm::DrogonDbException& e) {
        std::string what{e.base().what()};
        if (what.find("unique") != std::string::npos ||
            what.find("duplicate") != std::string::npos) {
            throw ConflictError("username '" + username + "' already exists");
        }
        throw;
    }
}

std::optional<User> UserRepository::find_by_username(const std::string& username) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id,  username,  password_hash,  created_at,  updated_at"
        " FROM users WHERE username = $1", 
        username.c_str());
    if (rows.empty()) return std::nullopt;
    const auto& r = rows[0];
    return User{
        r["id"].as<std::string>(),
        r["username"].as<std::string>(),
        r["password_hash"].as<std::string>(),
        r["created_at"].as<std::string>(),
        r["updated_at"].as<std::string>()
    };
}

std::optional<User> UserRepository::find_by_id(const std::string& id) {
    auto db = db::get_db_client();
    auto rows = db->execSqlSync(
        "SELECT id,  username,  password_hash,  created_at,  updated_at"
        " FROM users WHERE id = $1::UUID", 
        id.c_str());
    if (rows.empty()) return std::nullopt;
    const auto& r = rows[0];
    return User{
        r["id"].as<std::string>(),
        r["username"].as<std::string>(),
        r["password_hash"].as<std::string>(),
        r["created_at"].as<std::string>(),
        r["updated_at"].as<std::string>()
    };
}

void UserRepository::update_password(const std::string& id,
                                     const std::string& new_hash) {
    auto db = db::get_db_client();
    db->execSqlSync(
        "UPDATE users SET password_hash = $1,  updated_at = NOW()"
        " WHERE id = $2::UUID", 
        new_hash.c_str(),  id.c_str());
}

}  // namespace zion
