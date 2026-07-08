#include "Game.hpp"

#include "MoveRules.hpp"
#include "Piece.hpp"

namespace kfc::logic {

Game::Game(std::vector<Row> initialRows) : board_(std::move(initialRows)) {}

void Game::handleClickCell(Position p) {
    if (isOver()) return;
    if (!board_.inBounds(p)) return;

    if (!selected_.has_value()) {
        trySelect(p);
        return;
    }

    if (tryReselectSameColor(p)) return;

    tryMove(p);
}

void Game::handleJumpCommand(Position p) {
    if (isOver()) return;
    if (!board_.inBounds(p)) return;
    if (!canJump(p)) return;

    pendingJump_ = PendingJump{p, board_.colorAt(p), clockMs_ + kJumpDurationMs};
}

void Game::advanceClock(long long ms) {
    clockMs_ += ms;
    applyArrivedMoves();
}

const Board& Game::board() const { return board_; }

bool Game::isOver() const { return winner_.has_value(); }

std::optional<char> Game::winner() const { return winner_; }

bool Game::isPending(Position p) const {
    return pendingMove_.has_value() && pendingMove_->from == p;
}

bool Game::hasPieceInTransit() const { return pendingMove_.has_value(); }

bool Game::isJumping(Position p) const {
    return pendingJump_.has_value() && pendingJump_->cell == p;
}

bool Game::canJump(Position p) const {
    if (board_.isEmpty(p)) return false;
    if (isPending(p)) return false;
    if (isJumping(p)) return false;
    return true;
}

bool Game::isEnemyJumpAt(Position p, char movingColor) const {
    return pendingJump_.has_value() && pendingJump_->cell == p && pendingJump_->color != movingColor;
}

void Game::trySelect(Position p) {
    if (!board_.isEmpty(p) && !isPending(p)) selected_ = p;
}

bool Game::tryReselectSameColor(Position p) {
    if (board_.isEmpty(p) || isPending(p) || !board_.sameColor(*selected_, p)) return false;
    selected_ = p;
    return true;
}

void Game::tryMove(Position p) {
    if (hasPieceInTransit()) return;
    if (isJumping(*selected_)) return;
    if (!isMoveLegal(board_, board_.pieceAt(*selected_), board_.colorAt(*selected_), *selected_, p)) return;

    pendingMove_ = PendingMove{*selected_, p, clockMs_ + travelDurationMs(*selected_, p)};
    selected_.reset();
}

void Game::applyArrivedMoves() {
    if (pendingMove_.has_value() && pendingMove_->arrivalMs <= clockMs_) {
        resolveArrivingMove(pendingMove_->from, pendingMove_->to);
        pendingMove_.reset();
    }
    if (pendingJump_.has_value() && pendingJump_->expiryMs <= clockMs_) {
        pendingJump_.reset();
    }
}

void Game::resolveArrivingMove(Position from, Position to) {
    if (isEnemyJumpAt(to, board_.colorAt(from))) {
        board_.removePiece(from);
        pendingJump_.reset();
        return;
    }
    checkForKingCapture(from, to);
    board_.movePiece(from, to);
    checkForPromotion(to);
}

void Game::checkForKingCapture(Position from, Position to) {
    if (board_.isRoyalAt(to)) {
        winner_ = board_.colorAt(from);
    }
}

void Game::checkForPromotion(Position to) {
    auto newSymbol = board_.pieceAt(to).promotionSymbol(board_.colorAt(to), to, board_.height());
    if (newSymbol) board_.transformPiece(to, *newSymbol);
}

}
