#include "realtime/real_time_arbiter.hpp"

#include <algorithm>
#include <utility>

namespace kfc::realtime {

namespace {

double fractionOf(int elapsedMs, int durationMs) {
    if (durationMs <= 0) return 1.0;
    return std::min(1.0, static_cast<double>(elapsedMs) / durationMs);
}

CellProgress progressOf(const Jump& jump) {
    return {std::nullopt, fractionOf(jump.elapsedMs(), jump.durationMs()),
            jump.elapsedMs()};
}

}

RealTimeArbiter::RealTimeArbiter(model::Board& board, MotionProfiles profiles)
    : board_(board), profiles_(std::move(profiles)) {}

bool RealTimeArbiter::startMotion(model::Position from, model::Position to) {
    if (hasActiveMotion()) {
        return false;
    }
    std::optional<model::Piece> mover = board_.pieceAt(from);
    if (!mover) {
        return false;
    }
    int durationMs =
        travelDurationMs(from, to, profiles_.squareTravelMs(mover->kind()));
    active_.emplace_back(mover->id(), from, to, durationMs);
    board_.setPieceState(from, model::PieceState::kMoving);
    return true;
}

bool RealTimeArbiter::startJump(model::Position cell) {
    if (isAirborneAt(cell)) {
        return false;
    }
    std::optional<model::Piece> jumper = board_.pieceAt(cell);
    if (!jumper) {
        return false;
    }
    airborne_.emplace_back(cell, profiles_.jumpDurationMs(jumper->kind()));
    board_.setPieceState(cell, model::PieceState::kAirborne);
    return true;
}

std::vector<ArrivalReport> RealTimeArbiter::advance(int deltaMs) {
    std::vector<ArrivalReport> reports;
    tickCooldowns(deltaMs);
    for (auto it = active_.begin(); it != active_.end();) {
        it->advance(deltaMs);
        if (it->hasArrived()) {
            if (std::optional<ArrivalReport> report = resolveArrival(*it)) {
                reports.push_back(*report);
            }
            it = active_.erase(it);
        } else {
            ++it;
        }
    }
    landAirborne(deltaMs, reports);
    return reports;
}

CellProgress RealTimeArbiter::progressAt(model::Position cell) const {
    for (const Motion& motion : active_) {
        if (motion.from() == cell) {
            return {motion.to(),
                    fractionOf(motion.elapsedMs(), motion.durationMs()),
                    motion.elapsedMs()};
        }
    }
    for (const Jump& jump : airborne_) {
        if (jump.cell() == cell) {
            return progressOf(jump);
        }
    }
    for (const Cooldown& cooldown : resting_) {
        if (cooldown.cell() == cell) {
            return {std::nullopt,
                    fractionOf(cooldown.elapsedMs(), cooldown.durationMs()),
                    cooldown.elapsedMs()};
        }
    }
    return {};
}

std::vector<LiftedPiece> RealTimeArbiter::liftedPieces() const {
    std::vector<LiftedPiece> lifted;
    for (const Jump& jump : airborne_) {
        if (!jump.isLifted()) continue;
        lifted.push_back({jump.lifted(), progressOf(jump)});
    }
    return lifted;
}

std::optional<ArrivalReport> RealTimeArbiter::resolveArrival(const Motion& motion) {
    model::Position from = motion.from();
    model::Position to = motion.to();

    std::optional<model::Piece> arriver = board_.pieceAt(from);
    if (!arriver || arriver->id() != motion.pieceId()) {
        return std::nullopt;
    }
    std::optional<model::Piece> occupant = board_.pieceAt(to);

    if (occupant && occupant->state() == model::PieceState::kAirborne &&
        arriver->color() != occupant->color()) {
        jumpAt(to)->lift(*occupant);
        board_.removePiece(to);
        board_.movePiece(from, to);
        startLongRest(board_.pieceAt(to)->id(), to);
        return ArrivalReport{to, std::nullopt, false, true};
    }

    if (occupant) {
        board_.removePiece(to);
    }
    board_.movePiece(from, to);
    startLongRest(board_.pieceAt(to)->id(), to);

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
            startShortRest(lander.id(), cell);

            bool kingCaptured =
                victim && victim->kind() == model::PieceKind::kKing;
            reports.push_back(ArrivalReport{cell, victim, kingCaptured, false});
        } else {
            startShortRest(board_.pieceAt(cell)->id(), cell);
        }
        it = airborne_.erase(it);
    }
}

void RealTimeArbiter::startShortRest(model::PieceId pieceId,
                                     model::Position cell) {
    board_.setPieceState(cell, model::PieceState::kShortResting);
    resting_.emplace_back(pieceId, cell, kShortRestMs);
}

void RealTimeArbiter::startLongRest(model::PieceId pieceId,
                                    model::Position cell) {
    board_.setPieceState(cell, model::PieceState::kLongResting);
    resting_.emplace_back(pieceId, cell, kLongRestMs);
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
            (piece->state() == model::PieceState::kShortResting ||
             piece->state() == model::PieceState::kLongResting)) {
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
