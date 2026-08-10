// tests/unit/util/HashTest.cc
#include "util/Hash.h"
#include <gtest/gtest.h>
#include <string>

using namespace zion::util;

TEST(HashTest, Sha256KnownValue) {
    // sha256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    auto h = sha256_hex("");
    EXPECT_EQ(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(HashTest, Sha256NonEmpty) {
    auto h = sha256_hex("hello");
    EXPECT_EQ(h.size(), 64u);
}

TEST(HashTest, SecureCompareEqual) {
    EXPECT_TRUE(secure_compare("abc", "abc"));
}

TEST(HashTest, SecureCompareNotEqual) {
    EXPECT_FALSE(secure_compare("abc", "xyz"));
}

TEST(HashTest, RandomTokenLength) {
    auto t = random_token(32);
    // hex encoding: 32 bytes → 64 chars
    EXPECT_EQ(t.size(), 64u);
}

TEST(HashTest, Argon2RoundTrip) {
    std::string pw = "supersecret";
    auto hash = argon2id_hash(pw, Argon2Params{65536, 1, 1});
    EXPECT_TRUE(argon2id_verify(pw, hash));
    EXPECT_FALSE(argon2id_verify("wrongpassword", hash));
}
