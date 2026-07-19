#include "view/game_snapshot.hpp"

#include "io/move_notation.hpp"
#include "product/game_state_view.hpp"

namespace kfc::view {

namespace {

std::vector<model::Position> collectMoveTargets(
    const engine::GameEngine& engine,
    const std::optional<model::Position>& selection) {
    if (!selection) return {};
    rules::Destinations destinations = engine.legalDestinationsFrom(*selection);
    return {destinations.begin(), destinations.end()};
}

// Wording the log is this seam's job: the log keeps records, the renderer draws
// strings, and the turn from one into the other happens here and only here.
std::vector<MoveLine> wordMoves(const std::vector<product::MoveRecord>& records,
                                int boardHeight) {
    std::vector<MoveLine> lines;
    lines.reserve(records.size());
    for (const product::MoveRecord& record : records) {
        lines.push_back({io::clockText(record.atMs),
                         io::notationOf(record, boardHeight)});
    }
    return lines;
}

std::vector<PlayerView> wordPlayers(
    const std::vector<product::PlayerSnapshot>& players, int boardHeight) {
    std::vector<PlayerView> worded;
    worded.reserve(players.size());
    for (const product::PlayerSnapshot& player : players) {
        worded.push_back({player.color, player.name, player.score,
                          wordMoves(player.moves, boardHeight)});
    }
    return worded;
}

}

GameSnapshot buildSnapshot(const engine::GameEngine& engine,
                           const std::optional<model::Position>& selection,
                           const product::ScoreBoard& scores,
                           const product::MoveLog& log) {
    product::GameStateView state = product::gameStateView(engine, scores, log);
    return {state.boardWidth,
            state.boardHeight,
            std::move(state.pieces),
            selection,
            collectMoveTargets(engine, selection),
            state.gameOver,
            wordPlayers(state.players, state.boardHeight)};
}

}
