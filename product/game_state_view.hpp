#pragma once

#include <optional>
#include <string>
#include <vector>

#include "engine/game_engine.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"

namespace kfc::product {

// One piece as it stands right now: where it is, what it is, and — for a piece
// mid-move — where it is headed and how far along it is.
struct PieceSnapshot {
    model::PieceKind kind;
    model::Color color;
    model::Position cell;
    model::PieceState state;
    std::optional<model::Position> movingTo;
    double progress = 0.0;
    int stateElapsedMs = 0;
};

// One player's standing: their score and the actions they have taken, kept as
// records — how a move should read is not this layer's business.
struct PlayerSnapshot {
    model::Color color;
    std::string name;
    int score;
    std::vector<MoveRecord> moves;
};

// A read-only projection of the whole game, assembled from the engine and the
// product observers. It carries no display or per-client concerns (no selection,
// no highlighting, no worded text), so both the GUI and the server build on it
// without either reaching into engine internals twice.
struct GameStateView {
    int boardWidth;
    int boardHeight;
    std::vector<PieceSnapshot> pieces;
    bool gameOver;
    std::vector<PlayerSnapshot> players;
};

GameStateView gameStateView(const engine::GameEngine& engine,
                            const ScoreBoard& scores, const MoveLog& log);

}
