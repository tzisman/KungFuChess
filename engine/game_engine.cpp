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

void GameEngine::addObserver(GameObserver& observer) {
    observers_.push_back(&observer);
}

void GameEngine::notifyAction(const ActionEvent& event) const {
    for (GameObserver* observer : observers_) observer->onAction(event);
}

void GameEngine::notifyCapture(const CaptureEvent& event) const {
    for (GameObserver* observer : observers_) observer->onCapture(event);
}

void GameEngine::notifyGameOver(const GameOverEvent& event) const {
    for (GameObserver* observer : observers_) observer->onGameOver(event);
}

void GameEngine::announceAction(const model::Piece& actor, model::Position to,
                                ActionKind action) const {
    notifyAction({actor.id(), actor.color(), actor.kind(), actor.cell(), to,
                  action, state_.elapsedMs()});
}

MoveResult GameEngine::checkMove(model::Position from, model::Position to) const {
    if (state_.isOver()) {
        return {false, kReasonGameOver};
    }

    rules::MoveValidation validation = ruleEngine_.validate(state_.board(), from, to);
    if (!validation.isValid) {
        return {false, rules::reasonCode(validation.reason)};
    }

    if (state_.board().pieceAt(from)->state() != model::PieceState::kIdle) {
        return {false, kReasonNotIdle};
    }

    return {true, rules::reasonCode(rules::Reason::kOk)};
}

MoveResult GameEngine::requestMove(model::Position from, model::Position to) {
    MoveResult result = checkMove(from, to);
    if (!result.isAccepted) {
        return result;
    }

    model::Piece mover = state_.board().pieceAt(from).value();
    arbiter_.startMotion(from, to);
    announceAction(mover, to, ActionKind::kMove);
    return result;
}

rules::Destinations GameEngine::legalDestinationsFrom(model::Position from) const {
    rules::Destinations accepted;
    for (model::Position to : ruleEngine_.legalDestinations(state_.board(), from)) {
        if (checkMove(from, to).isAccepted) {
            accepted.insert(to);
        }
    }
    return accepted;
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
    announceAction(*piece, cell, ActionKind::kJump);
    return {true, rules::reasonCode(rules::Reason::kOk)};
}

// Whoever now stands on the square is the piece that took it, on both the
// arrival and the landing path, so the board itself names the captor and the
// arbiter need not carry it.
void GameEngine::announceCapture(const realtime::ArrivalReport& report) const {
    if (!report.captured) return;

    std::optional<model::Piece> captor = state_.board().pieceAt(report.destination);
    if (!captor) return;

    notifyCapture({*report.captured, captor->color(), state_.elapsedMs()});
}

void GameEngine::endGame(const realtime::ArrivalReport& report) {
    state_.markOver();

    std::optional<model::Piece> winner = state_.board().pieceAt(report.destination);
    if (!winner) return;

    notifyGameOver({winner->color(), state_.elapsedMs()});
}

void GameEngine::applyPromotion(const realtime::ArrivalReport& report) {
    if (auto promoted = rules::promotedKind(state_.board(), state_.board().pieceAt(report.destination).value())) {
        state_.board().setPieceKind(report.destination, *promoted);
    }
}

// The capture is announced before the game is called, so taking the king is
// still scored as the capture it is rather than vanishing into the win.
void GameEngine::advance(int ms) {
    state_.advanceClock(ms);
    std::vector<realtime::ArrivalReport> reports = arbiter_.advance(ms);
    for (const realtime::ArrivalReport& report : reports) {
        announceCapture(report);
        if (report.kingCaptured) {
            endGame(report);
            return;
        }
        if (!report.landed) {
            continue;
        }
        applyPromotion(report);
    }
}

const model::Board& GameEngine::board() const { return state_.board(); }

realtime::CellProgress GameEngine::progressAt(model::Position cell) const {
    return arbiter_.progressAt(cell);
}

std::vector<realtime::LiftedPiece> GameEngine::liftedPieces() const {
    return arbiter_.liftedPieces();
}

bool GameEngine::isOver() const { return state_.isOver(); }

}
