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

int travelDurationMs(model::Position from, model::Position to) {
    return cellSteps(from, to) * kSquareTravelMs;
}

Motion::Motion(model::Position from, model::Position to)
    : from_(from), to_(to), durationMs_(travelDurationMs(from, to)) {}

void Motion::advance(int deltaMs) {
    elapsedMs_ += deltaMs;
}

}
