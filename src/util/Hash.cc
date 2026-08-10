#include "util/Hash.h"

#include <argon2.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace zion::util {

namespace {

std::string bytes_to_hex(const unsigned char* buf, std::size_t len) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(buf[i]);
    }
    return ss.str();
}

}  // namespace

std::string sha256_hex(std::string_view data) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), digest);
    return bytes_to_hex(digest, SHA256_DIGEST_LENGTH);
}

std::string sha256_file_hex(const std::string& path) {
    std::ifstream f{path, std::ios::binary};
    if (!f) throw std::runtime_error("sha256_file_hex: cannot open " + path);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    char buf[65536];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        EVP_DigestUpdate(ctx, buf, static_cast<std::size_t>(f.gcount()));
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen = 0;
    EVP_DigestFinal_ex(ctx, digest, &dlen);
    EVP_MD_CTX_free(ctx);

    return bytes_to_hex(digest, dlen);
}

bool secure_compare(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    // Constant-time comparison via OpenSSL
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

std::string argon2id_hash(std::string_view password, const Argon2Params& p) {
    // Argon2id encoded output is at most ~128 bytes
    constexpr std::size_t kEncodedLen = 256;
    std::vector<char> encoded(kEncodedLen, '\0');

    // Generate random 16-byte salt
    unsigned char salt[16];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        throw std::runtime_error("argon2id_hash: RAND_bytes failed");
    }

    int rc = argon2id_hash_encoded(
        p.iter, p.mem_kib, p.parallel,
        password.data(), password.size(),
        salt, sizeof(salt),
        32,  // hash length (bytes)
        encoded.data(), kEncodedLen);

    if (rc != ARGON2_OK) {
        throw std::runtime_error(
            std::string("argon2id_hash failed: ") + argon2_error_message(rc));
    }

    return std::string{encoded.data()};
}

bool argon2id_verify(std::string_view password, const std::string& encoded_hash) {
    int rc = ::argon2id_verify(encoded_hash.c_str(),
                               password.data(), password.size());
    return rc == ARGON2_OK;
}

std::string random_hex(std::size_t byte_count) {
    std::vector<unsigned char> buf(byte_count);
    if (RAND_bytes(buf.data(), static_cast<int>(byte_count)) != 1) {
        throw std::runtime_error("random_hex: RAND_bytes failed");
    }
    return bytes_to_hex(buf.data(), byte_count);
}

std::string random_token(std::size_t byte_count) {
    // URL-safe base64-like encoding using hex is simpler and safe enough
    return random_hex(byte_count);
}

}  // namespace zion::util
