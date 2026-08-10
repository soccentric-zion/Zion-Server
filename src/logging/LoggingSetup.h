#pragma once
// LoggingSetup — initialise spdlog with async JSON output to stdout.
// Call setup_logging() once at startup before any log::* calls.

#include <string_view>

namespace zion::logging {

// Initialise async spdlog logger (JSON, stdout).
// level: debug | info | warn | error | critical
void setup_logging(std::string_view level = "info");

// MDC-like helpers — set per-thread context fields that are appended to every
// log record emitted from that thread.
void set_context(std::string_view key, std::string_view value);
void clear_context();

}  // namespace zion::logging
