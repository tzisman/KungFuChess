#pragma once

#include <map>
#include <vector>

#include "engine/game_observer.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::product {

// One action a player commanded, dated by the game clock.
struct MoveRecord {
    engine::ActionKind action;
    model::PieceKind kind;
    model::Position from;
    model::Position to;
    int atMs;
};

// What each player has done, in the order they did it. It keeps the actions as
// records rather than as text: how a move should read is not the game's
// business.
class MoveLog : public engine::GameObserver {
public:
    void onAction(const engine::ActionEvent& event) override;

    const std::vector<MoveRecord>& movesOf(model::Color color) const;

private:
    std::map<model::Color, std::vector<MoveRecord>> entries_;
};

}
