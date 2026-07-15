#include "realtime/motion.hpp"

#include <cstdlib>

namespace kfc::realtime {

namespace {

int cellSteps(model::Position from, model::Position to) {
    int dRow = std::abs(to.row - from.row);
    int dCol = std::abs(to.col - from.col);
    return dRow > dCol ? dRow : dCol;
}

}

int travelDurationMs(model::Position from, model::Position to,
                     int squareTravelMs) {
    return cellSteps(from, to) * squareTravelMs;
}

void MotionProfiles::setTiming(model::PieceKind kind, int squareTravelMs,
                               int jumpDurationMs) {
    timings_[kind] = Timing{squareTravelMs, jumpDurationMs};
}

int MotionProfiles::squareTravelMs(model::PieceKind kind) const {
    auto it = timings_.find(kind);
    return it == timings_.end() ? kSquareTravelMs : it->second.squareTravelMs;
}

int MotionProfiles::jumpDurationMs(model::PieceKind kind) const {
    auto it = timings_.find(kind);
    return it == timings_.end() ? kJumpDurationMs : it->second.jumpDurationMs;
}

Motion::Motion(model::PieceId pieceId, model::Position from,
               model::Position to, int durationMs)
    : pieceId_(pieceId), from_(from), to_(to), durationMs_(durationMs) {}

void Motion::advance(int deltaMs) {
    elapsedMs_ += deltaMs;
}

Jump::Jump(model::Position cell, int durationMs)
    : cell_(cell), durationMs_(durationMs) {}

void Jump::advance(int deltaMs) {
    elapsedMs_ += deltaMs;
}

Cooldown::Cooldown(model::PieceId pieceId, model::Position cell,
                   int durationMs)
    : pieceId_(pieceId), cell_(cell), durationMs_(durationMs) {}

void Cooldown::advance(int deltaMs) {
    elapsedMs_ += deltaMs;
}

}
