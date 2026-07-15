#include "engine/game_engine.hpp"

#include <optional>
#include <utility>
#include <vector>

#include "model/piece.hpp"
#include "rules/piece_rules.hpp"

namespace kfc::engine {

GameEngine::GameEngine(model::Board board, realtime::MotionProfiles profiles)
    : state_(std::move(board)),
      arbiter_(state_.board(), std::move(profiles)) {}

MoveResult GameEngine::requestMove(model::Position from, model::Position to) {
    if (state_.isOver()) {
        return {false, kReasonGameOver};
    }

    if (arbiter_.hasActiveMotion()) {
        return {false, kReasonMotionInProgress};
    }

    rules::MoveValidation validation = ruleEngine_.validate(state_.board(), from, to);
    if (!validation.isValid) {
        return {false, rules::reasonCode(validation.reason)};
    }

    if (state_.board().pieceAt(from)->state() != model::PieceState::kIdle) {
        return {false, kReasonNotIdle};
    }

    arbiter_.startMotion(from, to);
    return {true, rules::reasonCode(rules::Reason::kOk)};
}

MoveResult GameEngine::requestJump(model::Position cell) {
    if (state_.isOver()) {
        return {false, kReasonGameOver};
    }

    std::optional<model::Piece> piece = state_.board().pieceAt(cell);
    if (!piece) {
        return {false, kReasonNoPiece};
    }
    if (piece->state() != model::PieceState::kIdle) {
        return {false, kReasonNotIdle};
    }

    arbiter_.startJump(cell);
    return {true, rules::reasonCode(rules::Reason::kOk)};
}

void GameEngine::advance(int ms) {
    std::vector<realtime::ArrivalReport> reports = arbiter_.advance(ms);
    for (const realtime::ArrivalReport& report : reports) {
        if (report.kingCaptured) {
            state_.markOver();
            return;
        }
        if (!report.landed) {
            continue;
        }
        if (auto promoted = rules::promotedKind(state_.board(), state_.board().pieceAt(report.destination).value())) {
            state_.board().setPieceKind(report.destination, *promoted);
        }
    }
}

const model::Board& GameEngine::board() const { return state_.board(); }

realtime::CellProgress GameEngine::progressAt(model::Position cell) const {
    return arbiter_.progressAt(cell);
}

bool GameEngine::isOver() const { return state_.isOver(); }

}
