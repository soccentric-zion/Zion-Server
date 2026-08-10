// tests/unit/util/UuidTest.cc
#include "util/Uuid.h"
#include <gtest/gtest.h>

using namespace zion::util;

TEST(UuidTest, GenerateIsValid) {
    auto u = generate_uuid();
    EXPECT_TRUE(is_valid_uuid(u));
}

TEST(UuidTest, UniquenessOfGenerated) {
    auto a = generate_uuid();
    auto b = generate_uuid();
    EXPECT_NE(a, b);
}

TEST(UuidTest, InvalidUuid) {
    EXPECT_FALSE(is_valid_uuid("not-a-uuid"));
    EXPECT_FALSE(is_valid_uuid(""));
}
