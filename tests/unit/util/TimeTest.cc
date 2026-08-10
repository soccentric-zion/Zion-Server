// tests/unit/util/TimeTest.cc
#include "util/Time.h"
#include <gtest/gtest.h>
#include <string>

using namespace zion::util;

TEST(TimeTest, UtcNowIso8601Format) {
    auto t = utc_now_iso8601();
    // Basic format check: YYYY-MM-DDTHH:MM:SS.xxxxxxZ
    EXPECT_GT(t.size(), 20u);
    EXPECT_EQ(t.back(), 'Z');
}

TEST(TimeTest, UtcNowUsIsPositive) {
    auto us = utc_now_us();
    EXPECT_GT(us, 0);
}
