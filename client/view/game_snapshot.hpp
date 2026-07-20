#pragma once

#include <optional>
#include <string>
#include <vector>

#include "engine/game_engine.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "product/game_state_view.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"

namespace kfc::view {

// A drawn piece is exactly the read-model's piece: the renderer needs nothing
// the neutral projection does not already carry.
using PieceView = product::PieceSnapshot;

// One logged action, already worded for the two columns it is shown in.
struct MoveLine {
    std::string time;
    std::string move;
};

// Everything shown about one player. The name is the player's to give; until
// players have names of their own it is simply what their colour is called.
struct PlayerView {
    model::Color color;
    std::string name;
    int score;
    std::vector<MoveLine> moves;
};

struct GameSnapshot {
    int boardWidth;
    int boardHeight;
    std::vector<PieceView> pieces;
    std::optional<model::Position> selectedCell;
    std::vector<model::Position> moveTargets;
    bool gameOver;
    std::vector<PlayerView> players;
};

GameSnapshot buildSnapshot(const engine::GameEngine& engine,
                           const std::optional<model::Position>& selection,
                           const product::ScoreBoard& scores,
                           const product::MoveLog& log);

// The engine-free path: build the drawn snapshot straight from a neutral
// read-model. A networked client, which holds no engine, decodes a
// GameStateView off the wire and renders through this. The selection and move
// targets are the client's own local UI, not part of the shared state.
GameSnapshot buildSnapshot(product::GameStateView state,
                           const std::optional<model::Position>& selection,
                           std::vector<model::Position> moveTargets);

}
