#pragma once

#include <optional>
#include <vector>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"

namespace kfc::realtime {


struct ArrivalReport {
    model::Position destination;
    std::optional<model::Piece> captured;
    bool kingCaptured = false;
    bool landed = true;
};

// How far along a cell's piece is in whatever it is doing right now: the cell
// it is travelling toward (if any), the fraction of the way through, and how
// long it has been in its current state. An unoccupied or idle cell reports
// zero progress.
struct CellProgress {
    std::optional<model::Position> movingTo;
    double progress = 0.0;
    int stateElapsedMs = 0;
};

// A piece that is airborne over a cell an enemy has since taken. The board
// holds one piece per cell, so it is off the board until it lands; anything
// that shows the game still has to account for it.
struct LiftedPiece {
    model::Piece piece;
    CellProgress progress;
};

class RealTimeArbiter {
public:
    explicit RealTimeArbiter(model::Board& board, MotionProfiles profiles = {});

    bool hasActiveMotion() const { return !active_.empty(); }

    bool startMotion(model::Position from, model::Position to);
    bool startJump(model::Position cell);
    std::vector<ArrivalReport> advance(int deltaMs);

    CellProgress progressAt(model::Position cell) const;
    std::vector<LiftedPiece> liftedPieces() const;

private:
    void resolveArrivedHops(std::vector<ArrivalReport>& reports);
    std::optional<ArrivalReport> resolveHop(model::PieceId pieceId);
    Motion* earliestArrivedMotion();
    Motion* motionFor(model::PieceId pieceId);
    void removeMotion(model::PieceId pieceId);
    bool hasMotionFor(model::PieceId pieceId) const;
    void landAirborne(int deltaMs, std::vector<ArrivalReport>& reports);
    void startShortRest(model::PieceId pieceId, model::Position cell);
    void startLongRest(model::PieceId pieceId, model::Position cell);
    void tickCooldowns(int deltaMs);
    bool isAirborneAt(model::Position cell) const;
    Jump* jumpAt(model::Position cell);

    model::Board& board_;
    MotionProfiles profiles_;
    std::vector<Motion> active_;
    std::vector<Jump> airborne_;
    std::vector<Cooldown> resting_;
};

}
