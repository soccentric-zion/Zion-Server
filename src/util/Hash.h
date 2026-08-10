#pragma once
// util/Hash.h — cryptographic hash helpers used by auth and OTA verification.
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace zion::util {

// SHA-256 hex digest of the given data.
std::string sha256_hex(std::string_view data);

// SHA-256 hex digest of a file's contents.  Throws on read failure.
std::string sha256_file_hex(const std::string& path);

// Constant-time string comparison (mitigates timing attacks).
bool secure_compare(std::string_view a, std::string_view b);

// ── argon2id ─────────────────────────────────────────────────────────────────
struct Argon2Params {
    uint32_t mem_kib  = 65536;
    uint32_t iter     = 3;
    uint32_t parallel = 4;
};

// Hash a password using argon2id.  Returns an encoded hash string.
std::string argon2id_hash(std::string_view password, const Argon2Params& p);

// Verify a plaintext password against a previously generated encoded hash.
bool argon2id_verify(std::string_view password, const std::string& encoded_hash);

// Generate a cryptographically-random hex string of byte_count bytes.
std::string random_hex(std::size_t byte_count);

// Generate a URL-safe random token string of byte_count bytes.
std::string random_token(std::size_t byte_count = 32);

}  // namespace zion::util
