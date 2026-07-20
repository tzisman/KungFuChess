#include "product/score_board.hpp"

#include "model/piece_cost.hpp"

namespace kfc::product {
namespace {

constexpr int kNoScore = 0;

}  // namespace

void ScoreBoard::subscribeTo(bus::EventBus& bus) {
    bus.subscribe<engine::CaptureEvent>(
        [this](const engine::CaptureEvent& event) { onCapture(event); });
}

void ScoreBoard::onCapture(const engine::CaptureEvent& event) {
    scores_[event.capturedBy] += model::costOf(event.victim.kind());
}

int ScoreBoard::scoreOf(model::Color color) const {
    auto found = scores_.find(color);
    return found == scores_.end() ? kNoScore : found->second;
}

}
