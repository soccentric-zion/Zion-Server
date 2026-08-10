#pragma once
// services/TargetService.h — board registration and heartbeat.
#include "repositories/TargetRepository.h"
#include "repositories/AuditRepository.h"
#include <optional>
#include <string>

namespace zion {

class TargetService {
public:
    std::optional<Board> get_board();
    Board register_board(const Board& b, const std::string& actor_user_id);
    void update_board(const Board& b, const std::string& actor_user_id);
    void heartbeat(const std::string& board_id, const std::string& status);
    void update_inventory(const std::string& board_id, const std::string& inventory_json);
    void remove_board(const std::string& actor_user_id);

private:
    TargetRepository target_repo_;
    AuditRepository  audit_repo_;
};

}  // namespace zion
