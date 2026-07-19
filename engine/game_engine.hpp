#pragma once

#include <string>
#include <vector>

#include "engine/game_observer.hpp"
#include "model/board.hpp"
#include "model/game_state.hpp"
#include "model/position.hpp"
#include "realtime/real_time_arbiter.hpp"
#include "rules/rule_engine.hpp"

namespace kfc::engine {

inline constexpr char kReasonGameOver[] = "game_over";
inline constexpr char kReasonNoPiece[] = "no_piece";
inline constexpr char kReasonNotIdle[] = "not_idle";

struct MoveResult {
    bool isAccepted;
    std::string reason;
};

class GameEngine {
public:
    explicit GameEngine(model::Board board,
                        realtime::MotionProfiles profiles = {});

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;

    // Follows the game from the outside. The engine reports what happened and
    // knows nothing of what an observer does with it, which is what lets a
    // scoreboard or a log exist without the rules ever hearing of them. The
    // observer must outlive the engine.
    void addObserver(GameObserver& observer);

    MoveResult requestMove(model::Position from, model::Position to);
    MoveResult requestJump(model::Position cell);
    void advance(int ms);

    rules::Destinations legalDestinationsFrom(model::Position from) const;

    const model::Board& board() const;
    realtime::CellProgress progressAt(model::Position cell) const;
    std::vector<realtime::LiftedPiece> liftedPieces() const;
    bool isOver() const;

private:
    MoveResult checkMove(model::Position from, model::Position to) const;

    void announceAction(const model::Piece& actor, model::Position to,
                        ActionKind action) const;
    void announceCapture(const realtime::ArrivalReport& report) const;
    void endGame(const realtime::ArrivalReport& report);
    void applyPromotion(const realtime::ArrivalReport& report);

    void notifyAction(const ActionEvent& event) const;
    void notifyCapture(const CaptureEvent& event) const;
    void notifyGameOver(const GameOverEvent& event) const;

    model::GameState state_;
    rules::RuleEngine ruleEngine_;
    realtime::RealTimeArbiter arbiter_;
    std::vector<GameObserver*> observers_;
};

}
