#pragma once
// MigrationRunner — applies SQL migration files from migrations/ at startup.
// Migrations are applied in lexicographic order of filename.
// Idempotent: already-applied versions tracked in schema_migrations table.
// Aborts startup with std::runtime_error on any failure.

#include <string>

namespace zion::db {

class MigrationRunner {
public:
    // migrations_dir: path to the directory containing *.sql migration files.
    explicit MigrationRunner(std::string migrations_dir);

    // Run all pending migrations against the provided Drogon DbClient.
    // Throws std::runtime_error if any migration fails.
    void run();

private:
    std::string migrations_dir_;
};

}  // namespace zion::db
