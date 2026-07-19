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
    std::optional<model::Piece> mover = board_.pieceAt(from);
    if (!mover || hasMotionFor(mover->id())) {
        return false;
    }
    active_.emplace_back(
        mover->id(), from,
        buildRoute(from, to, profiles_.squareTravelMs(mover->kind())));
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
    for (Motion& motion : active_) {
        motion.advance(deltaMs);
    }
    resolveArrivedHops(reports);
    landAirborne(deltaMs, reports);
    return reports;
}

CellProgress RealTimeArbiter::progressAt(model::Position cell) const {
    for (const Motion& motion : active_) {
        if (motion.currentCell() == cell) {
            return {motion.nextCell(), motion.hopProgress(), motion.elapsedMs()};
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

// A hop's overshoot past its own duration: the more a piece has run past the
// finish line, the earlier within the tick it arrived. This orders simultaneous
// arrivals so the later mover is the one that captures or is turned back.
int arrivalKey(const Motion& motion) {
    return motion.elapsedMs() - motion.durationMs();
}

// The arrival to settle next: the earliest-arrived hop, ties broken by piece id
// so the resolution order is deterministic.
Motion* RealTimeArbiter::earliestArrivedMotion() {
    Motion* earliest = nullptr;
    for (Motion& motion : active_) {
        if (!motion.hopArrived()) continue;
        if (earliest == nullptr || arrivalKey(motion) > arrivalKey(*earliest) ||
            (arrivalKey(motion) == arrivalKey(*earliest) &&
             motion.pieceId() < earliest->pieceId())) {
            earliest = &motion;
        }
    }
    return earliest;
}

void RealTimeArbiter::resolveArrivedHops(std::vector<ArrivalReport>& reports) {
    while (Motion* earliest = earliestArrivedMotion()) {
        if (std::optional<ArrivalReport> report = resolveHop(earliest->pieceId())) {
            reports.push_back(*report);
        }
    }
}

// Settles one arrived hop against whatever holds the cell it lands on. Different
// colours mean the arriver takes the square; the same colour turns it back one
// cell; an empty square lets it move on or, at the route's end, come to rest.
std::optional<ArrivalReport> RealTimeArbiter::resolveHop(model::PieceId pieceId) {
    Motion* motion = motionFor(pieceId);
    model::Position from = motion->currentCell();
    model::Position to = motion->nextCell();

    std::optional<model::Piece> arriver = board_.pieceAt(from);
    if (!arriver || arriver->id() != pieceId) {
        removeMotion(pieceId);
        return std::nullopt;
    }

    std::optional<model::Piece> occupant = board_.pieceAt(to);

    if (occupant && occupant->color() == arriver->color()) {
        startLongRest(pieceId, from);
        removeMotion(pieceId);
        return std::nullopt;
    }

    if (occupant) {
        if (occupant->state() == model::PieceState::kAirborne) {
            jumpAt(to)->lift(*occupant);
            board_.removePiece(to);
            board_.movePiece(from, to);
            startLongRest(pieceId, to);
            removeMotion(pieceId);
            return ArrivalReport{to, std::nullopt, false, true};
        }
        removeMotion(occupant->id());
        board_.removePiece(to);
        board_.movePiece(from, to);
        startLongRest(pieceId, to);
        removeMotion(pieceId);
        return ArrivalReport{to, occupant,
                             occupant->kind() == model::PieceKind::kKing, true};
    }

    board_.movePiece(from, to);
    if (motion->advanceToNextHop()) {
        return std::nullopt;
    }
    startLongRest(pieceId, to);
    removeMotion(pieceId);
    return ArrivalReport{to, std::nullopt, false, true};
}

Motion* RealTimeArbiter::motionFor(model::PieceId pieceId) {
    for (Motion& motion : active_) {
        if (motion.pieceId() == pieceId) return &motion;
    }
    return nullptr;
}

void RealTimeArbiter::removeMotion(model::PieceId pieceId) {
    for (auto it = active_.begin(); it != active_.end(); ++it) {
        if (it->pieceId() == pieceId) {
            active_.erase(it);
            return;
        }
    }
}

bool RealTimeArbiter::hasMotionFor(model::PieceId pieceId) const {
    for (const Motion& motion : active_) {
        if (motion.pieceId() == pieceId) return true;
    }
    return false;
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
