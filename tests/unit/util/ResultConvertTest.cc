// tests/unit/util/ResultConvertTest.cc — unit tests for ResultConvert utilities.
#include "util/ResultConvert.h"
#include <gtest/gtest.h>
#include <json/json.h>

using namespace zion::util;

// ── status_to_string / string_to_status ──────────────────────────────────────

TEST(StatusConvertTest, ToStringRoundTrip) {
    EXPECT_EQ(status_to_string(1), "PASS");
    EXPECT_EQ(status_to_string(2), "FAIL");
    EXPECT_EQ(status_to_string(3), "ERROR");
    EXPECT_EQ(status_to_string(4), "TIMEOUT");
    EXPECT_EQ(status_to_string(5), "ABORTED");
    EXPECT_EQ(status_to_string(7), "SKIPPED");
    EXPECT_EQ(status_to_string(0), "UNSPECIFIED");
}

TEST(StatusConvertTest, FromStringRoundTrip) {
    EXPECT_EQ(string_to_status("PASS"),        1);
    EXPECT_EQ(string_to_status("FAIL"),        2);
    EXPECT_EQ(string_to_status("ERROR"),       3);
    EXPECT_EQ(string_to_status("TIMEOUT"),     4);
    EXPECT_EQ(string_to_status("ABORTED"),     5);
    EXPECT_EQ(string_to_status("SKIPPED"),     7);
    EXPECT_EQ(string_to_status("UNSPECIFIED"), 0);
    EXPECT_EQ(string_to_status("unknown"),     0);
}

TEST(StatusConvertTest, LowercaseInput) {
    // string_to_status should be case-insensitive.
    EXPECT_EQ(string_to_status("pass"), 1);
    EXPECT_EQ(string_to_status("fail"), 2);
}

// ── severity_to_string / string_to_severity ──────────────────────────────────

TEST(SeverityConvertTest, ToStringRoundTrip) {
    EXPECT_EQ(severity_to_string(1), "INFO");
    EXPECT_EQ(severity_to_string(2), "WARNING");
    EXPECT_EQ(severity_to_string(3), "ERROR");
    EXPECT_EQ(severity_to_string(4), "CRITICAL");
    EXPECT_EQ(severity_to_string(0), "UNSPECIFIED");
}

TEST(SeverityConvertTest, FromStringRoundTrip) {
    EXPECT_EQ(string_to_severity("INFO"),        1);
    EXPECT_EQ(string_to_severity("WARNING"),     2);
    EXPECT_EQ(string_to_severity("ERROR"),       3);
    EXPECT_EQ(string_to_severity("CRITICAL"),    4);
    EXPECT_EQ(string_to_severity("UNSPECIFIED"), 0);
}

// ── typed_map_to_json ─────────────────────────────────────────────────────────

TEST(TypedMapTest, DoubleValue) {
    google::protobuf::Map<std::string, hal::common::TypedValue> m;
    hal::common::TypedValue tv;
    tv.set_double_value(42.5);
    m["throughput"] = tv;

    Json::Value j = parse_typed_map_json(typed_map_to_json(m));
    ASSERT_TRUE(j.isMember("throughput"));
    EXPECT_EQ(j["throughput"]["type"].asString(), "double");
    EXPECT_DOUBLE_EQ(j["throughput"]["value"].asDouble(), 42.5);
}

TEST(TypedMapTest, IntValue) {
    google::protobuf::Map<std::string, hal::common::TypedValue> m;
    hal::common::TypedValue tv;
    tv.set_int_value(1234);
    m["count"] = tv;

    Json::Value j = parse_typed_map_json(typed_map_to_json(m));
    ASSERT_TRUE(j.isMember("count"));
    EXPECT_EQ(j["count"]["type"].asString(), "int");
    EXPECT_EQ(j["count"]["value"].asInt64(), 1234);
}

TEST(TypedMapTest, StringValue) {
    google::protobuf::Map<std::string, hal::common::TypedValue> m;
    hal::common::TypedValue tv;
    tv.set_string_value("hello");
    m["label"] = tv;

    Json::Value j = parse_typed_map_json(typed_map_to_json(m));
    ASSERT_TRUE(j.isMember("label"));
    EXPECT_EQ(j["label"]["type"].asString(), "string");
    EXPECT_EQ(j["label"]["value"].asString(), "hello");
}

TEST(TypedMapTest, BoolValue) {
    google::protobuf::Map<std::string, hal::common::TypedValue> m;
    hal::common::TypedValue tv;
    tv.set_bool_value(true);
    m["ok"] = tv;

    Json::Value j = parse_typed_map_json(typed_map_to_json(m));
    ASSERT_TRUE(j.isMember("ok"));
    EXPECT_EQ(j["ok"]["type"].asString(), "bool");
    EXPECT_TRUE(j["ok"]["value"].asBool());
}

// ── parse_typed_map_json ──────────────────────────────────────────────────────

TEST(ParseTypedMapTest, EmptyString_ReturnsEmptyObject) {
    Json::Value j = parse_typed_map_json("");
    EXPECT_TRUE(j.isObject());
    EXPECT_EQ(j.size(), 0u);
}

TEST(ParseTypedMapTest, RoundTrip) {
    const std::string raw =
        R"({"throughput":{"type":"double","value":12345.6},"ok":{"type":"bool","value":true}})";
    Json::Value j = parse_typed_map_json(raw);
    ASSERT_TRUE(j.isMember("throughput"));
    EXPECT_DOUBLE_EQ(j["throughput"]["value"].asDouble(), 12345.6);
    ASSERT_TRUE(j.isMember("ok"));
    EXPECT_TRUE(j["ok"]["value"].asBool());
}
