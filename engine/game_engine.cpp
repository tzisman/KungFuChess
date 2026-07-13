#include "engine/game_engine.hpp"

#include <utility>
#include <vector>

#include "rules/piece_rules.hpp"

namespace kfc::engine {

GameSnapshot::GameSnapshot(const model::Board& board, bool over)
    : board_(board), over_(over) {}

int GameSnapshot::width() const { return board_.width(); }
int GameSnapshot::height() const { return board_.height(); }

std::optional<model::Piece> GameSnapshot::pieceAt(model::Position cell) const {
    return board_.pieceAt(cell);
}

bool GameSnapshot::isOver() const { return over_; }

GameEngine::GameEngine(model::Board board)
    : state_(std::move(board)), arbiter_(state_.board()) {}

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

    arbiter_.startMotion(from, to);
    return {true, rules::reasonCode(rules::Reason::kOk)};
}

void GameEngine::wait(int ms) {
    std::vector<realtime::ArrivalReport> reports = arbiter_.advance(ms);
    for (const realtime::ArrivalReport& report : reports) {
        if (report.kingCaptured) {
            state_.markOver();
            return;
        }
        if (auto promoted = rules::promotedKind(state_.board(), state_.board().pieceAt(report.destination).value())) {
            state_.board().setPieceKind(report.destination, *promoted);
        }
    }
}

GameSnapshot GameEngine::snapshot() const {
    return {state_.board(), state_.isOver()};
}

bool GameEngine::isOver() const { return state_.isOver(); }

}
