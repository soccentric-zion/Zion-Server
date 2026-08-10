#pragma once
// util/Uuid.h — UUID generation and validation helpers.
#include <string>
#include <string_view>

namespace zion::util {

// Generate a random UUID v4 string (lowercase hex, with hyphens).
std::string generate_uuid();

// Returns true if s is a well-formed UUID string.
bool is_valid_uuid(std::string_view s);

}  // namespace zion::util
