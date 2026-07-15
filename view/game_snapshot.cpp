#include "view/game_snapshot.hpp"

namespace kfc::view {

namespace {

std::vector<PieceView> collectPieces(const model::Board& board) {
    std::vector<PieceView> pieces;
    for (int row = 0; row < board.height(); ++row) {
        for (int col = 0; col < board.width(); ++col) {
            model::Position cell{row, col};
            std::optional<model::Piece> piece = board.pieceAt(cell);
            if (!piece) continue;
            pieces.push_back({piece->kind(), piece->color(), cell, piece->state()});
        }
    }
    return pieces;
}

}

GameSnapshot buildSnapshot(const model::Board& board, bool gameOver,
                           const std::optional<model::Position>& selection) {
    return {board.width(), board.height(), collectPieces(board), selection,
            gameOver};
}

}
