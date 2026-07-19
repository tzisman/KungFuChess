#include "product/game_state_view.hpp"

#include "model/board.hpp"
#include "realtime/real_time_arbiter.hpp"

namespace kfc::product {

namespace {

PieceSnapshot snapshotOf(const model::Piece& piece,
                         const realtime::CellProgress& progress) {
    return {piece.kind(),        piece.color(),    piece.cell(),
            piece.state(),       progress.movingTo, progress.progress,
            progress.stateElapsedMs};
}

void collectBoardPieces(const engine::GameEngine& engine,
                        std::vector<PieceSnapshot>& pieces) {
    const model::Board& board = engine.board();
    for (int row = 0; row < board.height(); ++row) {
        for (int col = 0; col < board.width(); ++col) {
            model::Position cell{row, col};
            std::optional<model::Piece> piece = board.pieceAt(cell);
            if (!piece) continue;
            pieces.push_back(snapshotOf(*piece, engine.progressAt(cell)));
        }
    }
}

// Appended last so a piece still in the air is drawn over the enemy that took
// its square from under it.
void collectLiftedPieces(const engine::GameEngine& engine,
                         std::vector<PieceSnapshot>& pieces) {
    for (const realtime::LiftedPiece& lifted : engine.liftedPieces()) {
        pieces.push_back(snapshotOf(lifted.piece, lifted.progress));
    }
}

std::vector<PieceSnapshot> collectPieces(const engine::GameEngine& engine) {
    std::vector<PieceSnapshot> pieces;
    collectBoardPieces(engine, pieces);
    collectLiftedPieces(engine, pieces);
    return pieces;
}

std::vector<PlayerSnapshot> collectPlayers(const engine::GameEngine& engine,
                                           const ScoreBoard& scores,
                                           const MoveLog& log) {
    std::vector<PlayerSnapshot> players;
    players.reserve(model::kAllColors.size());
    for (model::Color color : model::kAllColors) {
        players.push_back({color, model::nameOf(color), scores.scoreOf(color),
                           log.movesOf(color)});
    }
    return players;
}

}

GameStateView gameStateView(const engine::GameEngine& engine,
                            const ScoreBoard& scores, const MoveLog& log) {
    return {engine.board().width(), engine.board().height(),
            collectPieces(engine), engine.isOver(),
            collectPlayers(engine, scores, log)};
}

}
