#pragma once

#include "model/position.hpp"

namespace kfc::realtime {

constexpr int kCellSizePx = 100;
constexpr int kPieceSpeedPxPerSec = 100;
constexpr int kSquareTravelMs = kCellSizePx * 1000 / kPieceSpeedPxPerSec;

int travelDurationMs(model::Position from, model::Position to);


class Motion {
public:
    Motion(model::Position from, model::Position to);

    model::Position from() const { return from_; }
    model::Position to() const { return to_; }
    int durationMs() const { return durationMs_; }
    int elapsedMs() const { return elapsedMs_; }

    void advance(int deltaMs);
    bool hasArrived() const { return elapsedMs_ >= durationMs_; }

private:
    model::Position from_;
    model::Position to_;
    int durationMs_;
    int elapsedMs_ = 0;
};

}
