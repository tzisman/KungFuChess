#pragma once

#include <optional>
#include <vector>

#include "engine/game_engine.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::view {

struct PieceView {
    model::PieceKind kind;
    model::Color color;
    model::Position cell;
    model::PieceState state;
    std::optional<model::Position> movingTo;
    double progress = 0.0;
    int stateElapsedMs = 0;
};

struct GameSnapshot {
    int boardWidth;
    int boardHeight;
    std::vector<PieceView> pieces;
    std::optional<model::Position> selectedCell;
    bool gameOver;
};

GameSnapshot buildSnapshot(const engine::GameEngine& engine,
                           const std::optional<model::Position>& selection);

}
