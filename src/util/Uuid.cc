#include "util/Uuid.h"

#include <openssl/rand.h>

#include <cstdio>
#include <regex>
#include <stdexcept>

namespace zion::util {

std::string generate_uuid() {
    unsigned char bytes[16];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        throw std::runtime_error("generate_uuid: RAND_bytes failed");
    }
    // RFC 4122 version 4
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;

    char buf[37];
    std::snprintf(buf, sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
        "%02x%02x%02x%02x%02x%02x",
        bytes[0],  bytes[1],  bytes[2],  bytes[3],
        bytes[4],  bytes[5],
        bytes[6],  bytes[7],
        bytes[8],  bytes[9],
        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return buf;
}

bool is_valid_uuid(std::string_view s) {
    static const std::regex kUuidRe{
        "[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}",
        std::regex::icase
    };
    return std::regex_match(std::string{s}, kUuidRe);
}

}  // namespace zion::util
