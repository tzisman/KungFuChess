#pragma once

#include <optional>
#include <string>

#include "model/board.hpp"
#include "model/game_state.hpp"
#include "model/piece.hpp"
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

class GameSnapshot {
public:
    GameSnapshot(const model::Board& board, bool over);

    int width() const;
    int height() const;
    std::optional<model::Piece> pieceAt(model::Position cell) const;
    bool isOver() const;

private:
    const model::Board& board_;
    bool over_;
};

class GameEngine {
public:
    explicit GameEngine(model::Board board);

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;

    MoveResult requestMove(model::Position from, model::Position to);
    MoveResult requestJump(model::Position cell);
    void wait(int ms);

    GameSnapshot snapshot() const;
    bool isOver() const;

private:
    model::GameState state_;
    rules::RuleEngine ruleEngine_;
    realtime::RealTimeArbiter arbiter_;
};

}
