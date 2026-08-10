// tests/unit/services/ThresholdServiceTest.cc — unit tests for typed threshold comparison.
//
// We only test the static helpers (compare) here; evaluate() requires a live DB.
#include "services/ThresholdService.h"
#include <gtest/gtest.h>
#include <json/json.h>

using zion::ThresholdService;
using zion::ThresholdTypeError;

// Helper to build a TypedValue JSON object.
static Json::Value make_typed(const std::string& type, double val) {
    Json::Value v;
    v["type"]  = type;
    v["value"] = val;
    return v;
}

static Json::Value make_typed_str(const std::string& val) {
    Json::Value v;
    v["type"]  = "string";
    v["value"] = val;
    return v;
}

static Json::Value make_typed_bool(bool val) {
    Json::Value v;
    v["type"]  = "bool";
    v["value"] = val;
    return v;
}

// Access the private compare via a test helper subclass.
class ThresholdServiceTest : public ThresholdService {
public:
    static bool call_compare(const Json::Value& lhs, const std::string& op,
                              const Json::Value& rhs) {
        return compare(lhs, op, rhs);
    }
};

// ── Numeric ops ───────────────────────────────────────────────────────────────

TEST(ThresholdCompareTest, GT_True) {
    EXPECT_TRUE(ThresholdServiceTest::call_compare(make_typed("double", 10.0), "GT",
                                                    make_typed("double", 5.0)));
}

TEST(ThresholdCompareTest, GT_False) {
    EXPECT_FALSE(ThresholdServiceTest::call_compare(make_typed("double", 5.0), "GT",
                                                     make_typed("double", 10.0)));
}

TEST(ThresholdCompareTest, LT_True) {
    EXPECT_TRUE(ThresholdServiceTest::call_compare(make_typed("double", 3.0), "LT",
                                                    make_typed("double", 7.0)));
}

TEST(ThresholdCompareTest, GE_Equal) {
    EXPECT_TRUE(ThresholdServiceTest::call_compare(make_typed("double", 5.0), "GE",
                                                    make_typed("double", 5.0)));
}

TEST(ThresholdCompareTest, LE_Equal) {
    EXPECT_TRUE(ThresholdServiceTest::call_compare(make_typed("double", 5.0), "LE",
                                                    make_typed("double", 5.0)));
}

TEST(ThresholdCompareTest, EQ_Strings) {
    EXPECT_TRUE(ThresholdServiceTest::call_compare(make_typed_str("hello"), "EQ",
                                                    make_typed_str("hello")));
}

TEST(ThresholdCompareTest, NE_Strings) {
    EXPECT_TRUE(ThresholdServiceTest::call_compare(make_typed_str("hello"), "NE",
                                                    make_typed_str("world")));
}

TEST(ThresholdCompareTest, EQ_Bool) {
    EXPECT_TRUE(ThresholdServiceTest::call_compare(make_typed_bool(true), "EQ",
                                                    make_typed_bool(true)));
    EXPECT_FALSE(ThresholdServiceTest::call_compare(make_typed_bool(true), "EQ",
                                                     make_typed_bool(false)));
}

// ── Type mismatch ─────────────────────────────────────────────────────────────

TEST(ThresholdCompareTest, TypeMismatch_GT_Throws) {
    EXPECT_THROW(
        ThresholdServiceTest::call_compare(make_typed_str("hi"), "GT", make_typed("double", 5.0)),
        ThresholdTypeError);
}

TEST(ThresholdCompareTest, TypeMismatch_EQ_DifferentTypes_Throws) {
    EXPECT_THROW(
        ThresholdServiceTest::call_compare(make_typed_str("5"), "EQ", make_typed("double", 5.0)),
        ThresholdTypeError);
}

// ── Null inputs ───────────────────────────────────────────────────────────────

TEST(ThresholdCompareTest, NullLhs_ReturnsFalse) {
    EXPECT_FALSE(ThresholdServiceTest::call_compare(Json::nullValue, "GT",
                                                     make_typed("double", 5.0)));
}
