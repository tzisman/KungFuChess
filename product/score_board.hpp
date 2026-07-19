#pragma once

#include <map>

#include "bus/event_bus.hpp"
#include "engine/game_events.hpp"
#include "model/piece.hpp"

namespace kfc::product {

// Each player's score: the summed cost of the pieces that player has captured.
// It learns of captures by watching the game, so the rules keep no notion that
// anyone is counting.
class ScoreBoard {
public:
    void subscribeTo(bus::EventBus& bus);
    void onCapture(const engine::CaptureEvent& event);

    int scoreOf(model::Color color) const;

private:
    std::map<model::Color, int> scores_;
};

}
