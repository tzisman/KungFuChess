#include "realtime/real_time_arbiter.hpp"

namespace kfc::realtime {

RealTimeArbiter::RealTimeArbiter(model::Board& board) : board_(board) {}

bool RealTimeArbiter::startMotion(model::Position from, model::Position to) {
    if (hasActiveMotion()) {
        return false;
    }
    active_.emplace_back(from, to);
    board_.setPieceState(from, model::PieceState::kMoving);
    return true;
}

bool RealTimeArbiter::startJump(model::Position cell) {
    if (isAirborneAt(cell)) {
        return false;
    }
    airborne_.emplace_back(cell);
    board_.setPieceState(cell, model::PieceState::kAirborne);
    return true;
}

std::vector<ArrivalReport> RealTimeArbiter::advance(int deltaMs) {
    std::vector<ArrivalReport> reports;
    tickCooldowns(deltaMs);
    for (auto it = active_.begin(); it != active_.end();) {
        it->advance(deltaMs);
        if (it->hasArrived()) {
            reports.push_back(resolveArrival(*it));
            it = active_.erase(it);
        } else {
            ++it;
        }
    }
    landAirborne(deltaMs, reports);
    return reports;
}

ArrivalReport RealTimeArbiter::resolveArrival(const Motion& motion) {
    model::Position from = motion.from();
    model::Position to = motion.to();

    std::optional<model::Piece> arriver = board_.pieceAt(from);
    std::optional<model::Piece> occupant = board_.pieceAt(to);

    if (occupant && occupant->state() == model::PieceState::kAirborne &&
        arriver && arriver->color() != occupant->color()) {
        jumpAt(to)->lift(*occupant);
        board_.removePiece(to);
        board_.movePiece(from, to);
        startCooldown(board_.pieceAt(to)->id(), to);
        return ArrivalReport{to, std::nullopt, false, true};
    }

    if (occupant) {
        board_.removePiece(to);
    }
    board_.movePiece(from, to);
    startCooldown(board_.pieceAt(to)->id(), to);

    bool kingCaptured =
        occupant && occupant->kind() == model::PieceKind::kKing;

    return ArrivalReport{to, occupant, kingCaptured, true};
}

void RealTimeArbiter::landAirborne(int deltaMs,
                                   std::vector<ArrivalReport>& reports) {
    for (auto it = airborne_.begin(); it != airborne_.end();) {
        it->advance(deltaMs);
        if (!it->hasLanded()) {
            ++it;
            continue;
        }

        model::Position cell = it->cell();
        if (it->isLifted()) {
            std::optional<model::Piece> victim = board_.pieceAt(cell);
            board_.removePiece(cell);

            model::Piece lander = it->lifted();
            lander.setCell(cell);
            board_.addPiece(lander);
            startCooldown(lander.id(), cell);

            bool kingCaptured =
                victim && victim->kind() == model::PieceKind::kKing;
            reports.push_back(ArrivalReport{cell, victim, kingCaptured, false});
        } else {
            startCooldown(board_.pieceAt(cell)->id(), cell);
        }
        it = airborne_.erase(it);
    }
}

void RealTimeArbiter::startCooldown(model::PieceId pieceId,
                                    model::Position cell) {
    board_.setPieceState(cell, model::PieceState::kResting);
    resting_.emplace_back(pieceId, cell);
}

void RealTimeArbiter::tickCooldowns(int deltaMs) {
    for (auto it = resting_.begin(); it != resting_.end();) {
        it->advance(deltaMs);
        if (!it->hasElapsed()) {
            ++it;
            continue;
        }

        std::optional<model::Piece> piece = board_.pieceAt(it->cell());
        if (piece && piece->id() == it->pieceId() &&
            piece->state() == model::PieceState::kResting) {
            board_.setPieceState(it->cell(), model::PieceState::kIdle);
        }
        it = resting_.erase(it);
    }
}

bool RealTimeArbiter::isAirborneAt(model::Position cell) const {
    std::optional<model::Piece> piece = board_.pieceAt(cell);
    return piece && piece->state() == model::PieceState::kAirborne;
}

Jump* RealTimeArbiter::jumpAt(model::Position cell) {
    for (Jump& jump : airborne_) {
        if (jump.cell() == cell) return &jump;
    }
    return nullptr;
}

}
