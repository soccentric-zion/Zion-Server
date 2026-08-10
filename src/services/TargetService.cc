#include "services/TargetService.h"
#include "util/Errors.h"

#include <spdlog/spdlog.h>

namespace zion {

std::optional<Board> TargetService::get_board() {
    return target_repo_.get();
}

Board TargetService::register_board(const Board& b, const std::string& actor_user_id) {
    auto board = target_repo_.create(b);
    audit_repo_.log(actor_user_id, "board.registered", "board", board.id, "{}");
    spdlog::info("target: board registered id={} name={}", board.id, board.name);
    return board;
}

void TargetService::update_board(const Board& b, const std::string& actor_user_id) {
    target_repo_.update(b);
    audit_repo_.log(actor_user_id, "board.updated", "board", b.id, "{}");
}

void TargetService::heartbeat(const std::string& board_id, const std::string& status) {
    target_repo_.heartbeat(board_id, status);
}

void TargetService::update_inventory(const std::string& board_id,
                                      const std::string& inventory_json) {
    target_repo_.update_inventory(board_id, inventory_json);
    spdlog::info("target: inventory updated board_id={}", board_id);
}

void TargetService::remove_board(const std::string& actor_user_id) {
    auto board = target_repo_.get();
    target_repo_.remove();
    if (board) {
        audit_repo_.log(actor_user_id, "board.removed", "board", board->id, "{}");
    }
}

}  // namespace zion
