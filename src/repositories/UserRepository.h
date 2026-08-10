#pragma once
// repositories/UserRepository.h
#include <optional>
#include <string>
#include <vector>

namespace zion {

struct User {
    std::string id;
    std::string username;
    std::string password_hash;
    std::string created_at;
    std::string updated_at;
};

class UserRepository {
public:
    UserRepository() = default;

    // Create a new user.  Throws ConflictError if username exists.
    User create(const std::string& username, const std::string& password_hash);

    // Find by username.
    std::optional<User> find_by_username(const std::string& username);

    // Find by id.
    std::optional<User> find_by_id(const std::string& id);

    // Update password hash.
    void update_password(const std::string& id, const std::string& new_hash);
};

}  // namespace zion
