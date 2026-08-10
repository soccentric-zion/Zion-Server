#include "db/MigrationRunner.h"
#include "db/DbClient.h"

#include <drogon/orm/DbClient.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace zion::db {

namespace fs = std::filesystem;

MigrationRunner::MigrationRunner(std::string migrations_dir)
    : migrations_dir_(std::move(migrations_dir)) {}

void MigrationRunner::run() {
    auto db = get_db_client();

    // Ensure the migrations tracking table exists.
    {
        auto result = db->execSqlSync(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "  version    TEXT        NOT NULL PRIMARY KEY,"
            "  applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()"
            ")");
        (void)result;
    }

    // Fetch already-applied versions.
    std::set<std::string> applied;
    {
        auto rows = db->execSqlSync("SELECT version FROM schema_migrations ORDER BY version");
        for (const auto& row : rows) {
            applied.insert(row["version"].as<std::string>());
        }
    }

    // Collect and sort migration files.
    std::vector<fs::path> files;
    if (!fs::exists(migrations_dir_)) {
        spdlog::warn("MigrationRunner: migrations directory '{}' not found — skipping",
                     migrations_dir_);
        return;
    }
    for (const auto& entry : fs::directory_iterator(migrations_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sql") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());

    int applied_count = 0;
    for (const auto& file : files) {
        std::string version = file.stem().string();

        if (applied.count(version)) {
            spdlog::debug("MigrationRunner: skip {} (already applied)", version);
            continue;
        }

        // Read the SQL file.
        std::ifstream f{file};
        if (!f.is_open()) {
            throw std::runtime_error(
                "MigrationRunner: cannot open migration file: " + file.string());
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        std::string sql = ss.str();

        spdlog::info("MigrationRunner: applying {}", version);
        try {
            // Split SQL by semicolons and execute each statement individually.
            // Drogon's execSqlSync uses prepared statements which cannot contain
            // multiple commands. Naive split is sufficient for standard migrations.
            std::istringstream stmt_stream{sql};
            std::string stmt;
            std::string line;
            while (std::getline(stmt_stream, line)) {
                stmt += line + "\n";
                // Remove trailing whitespace to test for semicolons.
                auto trimmed = line;
                trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
                if (!trimmed.empty() && trimmed.back() == ';') {
                    // Strip the trailing semicolon — execSqlSync doesn't need it.
                    auto s = stmt;
                    auto semi = s.rfind(';');
                    if (semi != std::string::npos) s.erase(semi);
                    // Skip empty / comment-only statements.
                    auto non_ws = s.find_first_not_of(" \t\r\n");
                    if (non_ws == std::string::npos) { stmt.clear(); continue; }
                    auto trimmed_s = s.substr(non_ws);
                    if (trimmed_s.substr(0, 2) == "--") { stmt.clear(); continue; }
                    try {
                        auto res = db->execSqlSync(s);
                        (void)res;
                    } catch (const drogon::orm::DrogonDbException& e) {
                        throw std::runtime_error(
                            "MigrationRunner: migration " + version + " failed: " + e.base().what());
                    }
                    stmt.clear();
                }
            }
            // Execute any trailing statement without a trailing semicolon.
            if (!stmt.empty()) {
                auto non_ws = stmt.find_first_not_of(" \t\r\n");
                if (non_ws != std::string::npos) {
                    try {
                        auto res = db->execSqlSync(stmt);
                        (void)res;
                    } catch (const drogon::orm::DrogonDbException& e) {
                        throw std::runtime_error(
                            "MigrationRunner: migration " + version + " failed: " + e.base().what());
                    }
                }
            }
        } catch (const std::runtime_error&) {
            throw;
        } catch (const drogon::orm::DrogonDbException& e) {
            throw std::runtime_error(
                "MigrationRunner: migration " + version + " failed: " + e.base().what());
        }

        // Record the migration (already done inside the SQL, but also record
        // here for migrations that don't self-register).
        try {
            db->execSqlSync(
                "INSERT INTO schema_migrations (version) VALUES ($1) "
                "ON CONFLICT (version) DO NOTHING",
                version);
        } catch (...) {
            // Not fatal — the SQL file itself may have already inserted the row.
        }

        ++applied_count;
        spdlog::info("MigrationRunner: applied {}", version);
    }

    spdlog::info("MigrationRunner: {} migration(s) applied, {} already up to date",
                 applied_count, static_cast<int>(applied.size()));
}

}  // namespace zion::db
