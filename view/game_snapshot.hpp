#pragma once

#include <optional>
#include <vector>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::view {

struct PieceView {
    model::PieceKind kind;
    model::Color color;
    model::Position cell;
    model::PieceState state;
};

struct GameSnapshot {
    int boardWidth;
    int boardHeight;
    std::vector<PieceView> pieces;
    std::optional<model::Position> selectedCell;
    bool gameOver;
};

GameSnapshot buildSnapshot(const model::Board& board, bool gameOver,
                           const std::optional<model::Position>& selection);

}
