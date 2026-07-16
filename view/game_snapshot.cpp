#include "view/game_snapshot.hpp"

#include "model/board.hpp"
#include "realtime/real_time_arbiter.hpp"

namespace kfc::view {

namespace {

PieceView viewOf(const model::Piece& piece,
                 const realtime::CellProgress& progress) {
    return {piece.kind(),        piece.color(),    piece.cell(),
            piece.state(),       progress.movingTo, progress.progress,
            progress.stateElapsedMs};
}

void collectBoardPieces(const engine::GameEngine& engine,
                        std::vector<PieceView>& pieces) {
    const model::Board& board = engine.board();
    for (int row = 0; row < board.height(); ++row) {
        for (int col = 0; col < board.width(); ++col) {
            model::Position cell{row, col};
            std::optional<model::Piece> piece = board.pieceAt(cell);
            if (!piece) continue;
            pieces.push_back(viewOf(*piece, engine.progressAt(cell)));
        }
    }
}

// Appended last so a piece still in the air is drawn over the enemy that took
// its square from under it.
void collectLiftedPieces(const engine::GameEngine& engine,
                         std::vector<PieceView>& pieces) {
    for (const realtime::LiftedPiece& lifted : engine.liftedPieces()) {
        pieces.push_back(viewOf(lifted.piece, lifted.progress));
    }
}

std::vector<PieceView> collectPieces(const engine::GameEngine& engine) {
    std::vector<PieceView> pieces;
    collectBoardPieces(engine, pieces);
    collectLiftedPieces(engine, pieces);
    return pieces;
}

std::vector<model::Position> collectMoveTargets(
    const engine::GameEngine& engine,
    const std::optional<model::Position>& selection) {
    if (!selection) return {};
    rules::Destinations destinations = engine.legalDestinationsFrom(*selection);
    return {destinations.begin(), destinations.end()};
}

}

GameSnapshot buildSnapshot(const engine::GameEngine& engine,
                           const std::optional<model::Position>& selection) {
    return {engine.board().width(),
            engine.board().height(),
            collectPieces(engine),
            selection,
            collectMoveTargets(engine, selection),
            engine.isOver()};
}

}
