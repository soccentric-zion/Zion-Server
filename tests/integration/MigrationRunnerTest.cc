// tests/integration/MigrationRunnerTest.cc
// Requires a real PostgreSQL instance.  Set ZION_TEST_DSN env var.
#include "db/MigrationRunner.h"
#include <drogon/drogon.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

// Skip integration test if ZION_TEST_DSN is not set.
static const char* test_dsn() {
    return std::getenv("ZION_TEST_DSN");
}

class MigrationRunnerTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!test_dsn()) GTEST_SKIP() << "ZION_TEST_DSN not set";
        // Create a temp migrations dir with a single migration.
        migrations_dir_ = std::filesystem::temp_directory_path() / "zion_test_migrations";
        std::filesystem::create_directories(migrations_dir_);
        {
            std::ofstream f{migrations_dir_ / "0001_test.sql"};
            f << "BEGIN;\n"
              << "CREATE TABLE IF NOT EXISTS _test_marker (id SERIAL PRIMARY KEY);\n"
              << "COMMIT;\n";
        }
    }

    void TearDown() override {
        std::filesystem::remove_all(migrations_dir_);
    }

    std::filesystem::path migrations_dir_;
};

TEST_F(MigrationRunnerTest, RunsMigrationsOnce) {
    zion::db::MigrationRunner runner{migrations_dir_.string()};
    EXPECT_NO_THROW(runner.run());
    // Second run should be idempotent.
    EXPECT_NO_THROW(runner.run());
}
