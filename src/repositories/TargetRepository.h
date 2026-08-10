#pragma once
// repositories/TargetRepository.h — single board tracking.
#include <optional>
#include <string>

namespace zion {

struct Board {
    std::string id;
    std::string name;
    std::string hostname;
    std::string model;
    std::string os_version;
    std::string hal_version;
    std::string client_version;
    std::string tags_json;               // JSONB
    std::string hal_module_inventory_json; // JSONB
    std::string first_seen;
    std::string last_seen;
    std::string status;  // online | offline
};

class TargetRepository {
public:
    // Returns the single board row, or nullopt if not yet registered.
    std::optional<Board> get();

    // Register the board.  Throws ConflictError if already registered.
    Board create(const Board& b);

    // Patch mutable fields (name, hostname, tags).
    void update(const Board& b);

    // Update last_seen + status.
    void heartbeat(const std::string& id, const std::string& status);

    // Update hal_module_inventory.
    void update_inventory(const std::string& id, const std::string& inventory_json);

    // Remove the board record.
    void remove();
};

}  // namespace zion
