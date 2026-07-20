#include "realtime/motion.hpp"

#include <algorithm>

#include "model/geometry.hpp"

namespace kfc::realtime {

int travelDurationMs(model::Position from, model::Position to,
                     int squareTravelMs) {
    return model::cellDistance(from, to) * squareTravelMs;
}

std::vector<Step> buildRoute(model::Position from, model::Position to,
                             int squareTravelMs) {
    std::vector<Step> steps;
    model::Position prev = from;
    for (model::Position cell : model::pathCells(from, to)) {
        steps.push_back({cell, travelDurationMs(prev, cell, squareTravelMs)});
        prev = cell;
    }
    return steps;
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

Motion::Motion(model::PieceId pieceId, model::Position origin,
               std::vector<Step> steps)
    : pieceId_(pieceId), currentCell_(origin), steps_(std::move(steps)) {}

double Motion::hopProgress() const {
    int duration = durationMs();
    if (duration <= 0) return 1.0;
    return std::min(1.0, static_cast<double>(elapsedMs_) / duration);
}

void Motion::advance(int deltaMs) {
    elapsedMs_ += deltaMs;
}

bool Motion::advanceToNextHop() {
    int overshoot = elapsedMs_ - steps_[index_].durationMs;
    currentCell_ = steps_[index_].cell;
    ++index_;
    if (index_ >= steps_.size()) {
        return false;
    }
    elapsedMs_ = std::max(0, overshoot);
    return true;
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
