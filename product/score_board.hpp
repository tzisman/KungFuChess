#pragma once

#include <map>

#include "engine/game_observer.hpp"
#include "model/piece.hpp"

namespace kfc::product {

// Each player's score: the summed cost of the pieces that player has captured.
// It learns of captures by watching the game, so the rules keep no notion that
// anyone is counting.
class ScoreBoard : public engine::GameObserver {
public:
    void onCapture(const engine::CaptureEvent& event) override;

    int scoreOf(model::Color color) const;

private:
    std::map<model::Color, int> scores_;
};

}
