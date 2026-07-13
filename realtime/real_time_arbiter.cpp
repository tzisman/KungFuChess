#include "realtime/real_time_arbiter.hpp"

namespace kfc::realtime {

RealTimeArbiter::RealTimeArbiter(model::Board& board) : board_(board) {}

bool RealTimeArbiter::startMotion(model::Position from, model::Position to) {
    if (hasActiveMotion()) {
        return false;
    }
    active_.emplace_back(from, to);
    return true;
}

std::vector<ArrivalReport> RealTimeArbiter::advance(int deltaMs) {
    std::vector<ArrivalReport> reports;
    for (auto it = active_.begin(); it != active_.end();) {
        it->advance(deltaMs);
        if (it->hasArrived()) {
            reports.push_back(resolveArrival(*it));
            it = active_.erase(it);
        } else {
            ++it;
        }
    }
    return reports;
}

ArrivalReport RealTimeArbiter::resolveArrival(const Motion& motion) {
    model::Position from = motion.from();
    model::Position to = motion.to();

    std::optional<model::Piece> captured = board_.pieceAt(to);
    if (captured) {
        board_.removePiece(to);
    }
    board_.movePiece(from, to);

    bool kingCaptured =
        captured && captured->kind() == model::PieceKind::kKing;

    return ArrivalReport{to, captured, kingCaptured};
}

}
