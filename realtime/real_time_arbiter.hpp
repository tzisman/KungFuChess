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

class RealTimeArbiter {
public:
    explicit RealTimeArbiter(model::Board& board);

    bool hasActiveMotion() const { return !active_.empty(); }

    bool startMotion(model::Position from, model::Position to);
    bool startJump(model::Position cell);
    std::vector<ArrivalReport> advance(int deltaMs);

private:
    ArrivalReport resolveArrival(const Motion& motion);
    void landAirborne(int deltaMs, std::vector<ArrivalReport>& reports);
    bool isAirborneAt(model::Position cell) const;
    Jump* jumpAt(model::Position cell);

    model::Board& board_;
    std::vector<Motion> active_;
    std::vector<Jump> airborne_;
};

}
