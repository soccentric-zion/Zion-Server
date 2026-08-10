// Minimal hand-rolled test runner — no external framework needed.
// Each TEST() macro registers a case; main() runs them all and reports.

#include "internal/config/config.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// ── tiny test harness ─────────────────────────────────────────────────────────
static int g_pass = 0, g_fail = 0;
struct TestCase { std::string name; std::function<void()> fn; };
static std::vector<TestCase> g_tests;

#define TEST(name) \
    static void test_##name(); \
    static bool reg_##name = (g_tests.push_back({#name, test_##name}), true); \
    static void test_##name()

#define EXPECT_EQ(a, b) \
    do { if ((a) != (b)) { \
        std::fprintf(stderr, "  FAIL %s:%d: expected [%s] == [%s]\n", \
                     __FILE__, __LINE__, #a, #b); \
        throw std::runtime_error("assertion failed"); \
    }} while(0)

#define EXPECT_THROWS(expr) \
    do { bool threw = false; try { expr; } catch(...) { threw = true; } \
         if (!threw) throw std::runtime_error("expected exception not thrown for: " #expr); \
    } while(0)

// ── tests ─────────────────────────────────────────────────────────────────────
TEST(defaults_are_sane) {
    auto cfg = zion::config::defaults();
    EXPECT_EQ(cfg.server.grpc_listen,  std::string("0.0.0.0:50051"));
    EXPECT_EQ(cfg.server.http_listen,  std::string("127.0.0.1:8080"));
    EXPECT_EQ(cfg.storage.db_path,     std::string("/var/lib/zion/zion.db"));
    EXPECT_EQ(cfg.ingest.queue_capacity,          uint32_t(100000));
    EXPECT_EQ(cfg.ingest.batch_commit_rows,        uint32_t(1000));
    EXPECT_EQ(cfg.ingest.batch_commit_interval_ms, uint32_t(1000));
    EXPECT_EQ(cfg.logs.fts_sources.size(),         size_t(2));
    EXPECT_EQ(cfg.logs.fts_sources[0],             std::string("hal"));
    EXPECT_EQ(cfg.logs.fts_sources[1],             std::string("client"));
    EXPECT_EQ(cfg.metrics.enabled,                 true);
}

TEST(load_overrides_specified_keys) {
    auto tmp = std::filesystem::temp_directory_path() / "zion_test_load.toml";
    {
        std::ofstream f(tmp);
        f << "[server]\n"
          << "grpc_listen = \"localhost:9999\"\n"
          << "\n"
          << "[ingest]\n"
          << "queue_capacity = 5000\n";
    }
    auto cfg = zion::config::load(tmp.string());
    std::filesystem::remove(tmp);

    EXPECT_EQ(cfg.server.grpc_listen,      std::string("localhost:9999"));
    EXPECT_EQ(cfg.ingest.queue_capacity,   uint32_t(5000));
    // Unspecified keys must stay at defaults.
    EXPECT_EQ(cfg.server.http_listen,      std::string("127.0.0.1:8080"));
    EXPECT_EQ(cfg.ingest.batch_commit_rows, uint32_t(1000));
}

TEST(load_overrides_fts_sources) {
    auto tmp = std::filesystem::temp_directory_path() / "zion_test_fts.toml";
    {
        std::ofstream f(tmp);
        f << "[logs]\n"
          << "fts_sources = [\"hal\", \"custom\"]\n";
    }
    auto cfg = zion::config::load(tmp.string());
    std::filesystem::remove(tmp);

    EXPECT_EQ(cfg.logs.fts_sources.size(), size_t(2));
    EXPECT_EQ(cfg.logs.fts_sources[0],     std::string("hal"));
    EXPECT_EQ(cfg.logs.fts_sources[1],     std::string("custom"));
}

TEST(load_throws_on_missing_file) {
    EXPECT_THROWS(zion::config::load("/no/such/file.toml"));
}

TEST(load_throws_on_invalid_toml) {
    auto tmp = std::filesystem::temp_directory_path() / "zion_test_bad.toml";
    {
        std::ofstream f(tmp);
        f << "this is not valid toml @@@@\n";
    }
    bool threw = false;
    try { zion::config::load(tmp.string()); }
    catch (const std::runtime_error&) { threw = true; }
    std::filesystem::remove(tmp);
    if (!threw) throw std::runtime_error("expected parse error not thrown");
}

// ── main ──────────────────────────────────────────────────────────────────────
int main() {
    for (auto& tc : g_tests) {
        std::fprintf(stdout, "%-50s ", tc.name.c_str());
        try {
            tc.fn();
            std::fprintf(stdout, "PASS\n");
            ++g_pass;
        } catch (const std::exception& e) {
            std::fprintf(stdout, "FAIL — %s\n", e.what());
            ++g_fail;
        }
    }
    std::fprintf(stdout, "\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
