#pragma once

#include <optional>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"

namespace kfc::realtime {


struct ArrivalReport {
    bool pieceArrived = false;
    std::optional<model::Piece> captured;
    bool kingCaptured = false;
};

class RealTimeArbiter {
public:
    explicit RealTimeArbiter(model::Board& board);

    bool hasActiveMotion() const { return active_.has_value(); }

    bool startMotion(model::Position from, model::Position to);
    ArrivalReport advance(int deltaMs);

private:
    ArrivalReport resolveArrival();

    model::Board& board_;
    std::optional<Motion> active_;
};

}
