#include "view/game_snapshot.hpp"

#include "model/board.hpp"
#include "realtime/real_time_arbiter.hpp"

namespace kfc::view {

namespace {

std::vector<PieceView> collectPieces(const engine::GameEngine& engine) {
    const model::Board& board = engine.board();
    std::vector<PieceView> pieces;
    for (int row = 0; row < board.height(); ++row) {
        for (int col = 0; col < board.width(); ++col) {
            model::Position cell{row, col};
            std::optional<model::Piece> piece = board.pieceAt(cell);
            if (!piece) continue;
            realtime::CellProgress progress = engine.progressAt(cell);
            pieces.push_back({piece->kind(), piece->color(), cell,
                              piece->state(), progress.movingTo,
                              progress.progress, progress.stateElapsedMs});
        }
    }
    return pieces;
}

}

GameSnapshot buildSnapshot(const engine::GameEngine& engine,
                           const std::optional<model::Position>& selection) {
    return {engine.board().width(), engine.board().height(),
            collectPieces(engine), selection, engine.isOver()};
}

}
