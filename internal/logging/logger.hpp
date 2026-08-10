#pragma once

#include <initializer_list>
#include <string_view>
#include <utility>

namespace zion::log {

// A single structured field: {"key", "value"}.
using Field  = std::pair<std::string_view, std::string_view>;
using Fields = std::initializer_list<Field>;

// Set the minimum log level. Call once at startup before any logging.
// level: "debug" | "info" | "warn" | "error" | "critical"
void init(std::string_view level = "info");

// Core emit function. Each call writes one JSON line to stderr.
void write(std::string_view level, std::string_view msg, Fields fields = {});

// Convenience wrappers.
void debug   (std::string_view msg, Fields fields = {});
void info    (std::string_view msg, Fields fields = {});
void warn    (std::string_view msg, Fields fields = {});
void error   (std::string_view msg, Fields fields = {});
void critical(std::string_view msg, Fields fields = {});

} // namespace zion::log
