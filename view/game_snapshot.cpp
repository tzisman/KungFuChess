#include "view/game_snapshot.hpp"

namespace kfc::view {

namespace {

std::vector<PieceView> collectPieces(const engine::GameSnapshot& state) {
    std::vector<PieceView> pieces;
    for (int row = 0; row < state.height(); ++row) {
        for (int col = 0; col < state.width(); ++col) {
            model::Position cell{row, col};
            std::optional<model::Piece> piece = state.pieceAt(cell);
            if (!piece) continue;
            pieces.push_back({piece->kind(), piece->color(), cell, piece->state()});
        }
    }
    return pieces;
}

}

GameSnapshot buildSnapshot(const engine::GameSnapshot& state,
                           const std::optional<model::Position>& selection) {
    return {state.width(), state.height(), collectPieces(state), selection,
            state.isOver()};
}

}
