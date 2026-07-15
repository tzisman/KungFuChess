#pragma once

#include <string>

#include "model/board.hpp"
#include "model/game_state.hpp"
#include "model/position.hpp"
#include "realtime/real_time_arbiter.hpp"
#include "rules/rule_engine.hpp"

namespace kfc::engine {

inline constexpr char kReasonGameOver[] = "game_over";
inline constexpr char kReasonMotionInProgress[] = "motion_in_progress";
inline constexpr char kReasonNoPiece[] = "no_piece";
inline constexpr char kReasonNotIdle[] = "not_idle";

struct MoveResult {
    bool isAccepted;
    std::string reason;
};

class GameEngine {
public:
    explicit GameEngine(model::Board board);

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;

    MoveResult requestMove(model::Position from, model::Position to);
    MoveResult requestJump(model::Position cell);
    void wait(int ms);

    const model::Board& board() const;
    bool isOver() const;

private:
    model::GameState state_;
    rules::RuleEngine ruleEngine_;
    realtime::RealTimeArbiter arbiter_;
};

}
