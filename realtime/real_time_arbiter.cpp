#include "realtime/real_time_arbiter.hpp"

namespace kfc::realtime {

RealTimeArbiter::RealTimeArbiter(model::Board& board) : board_(board) {}

bool RealTimeArbiter::startMotion(model::Position from, model::Position to) {
    if (hasActiveMotion()) {
        return false;
    }
    active_.emplace(from, to);
    return true;
}

ArrivalReport RealTimeArbiter::advance(int deltaMs) {
    if (!active_) {
        return {};
    }
    active_->advance(deltaMs);
    if (!active_->hasArrived()) {
        return {};
    }
    return resolveArrival();
}

ArrivalReport RealTimeArbiter::resolveArrival() {
    model::Position from = active_->from();
    model::Position to = active_->to();

    std::optional<model::Piece> captured = board_.pieceAt(to);
    if (captured) {
        board_.removePiece(to);
    }
    board_.movePiece(from, to);

    bool kingCaptured =
        captured && captured->kind() == model::PieceKind::kKing;

    active_.reset();
    return ArrivalReport{true, captured, kingCaptured};
}

}
