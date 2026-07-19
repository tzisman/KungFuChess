#include <doctest/doctest.h>

#include <utility>

#include "engine/game_engine.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "product/game_state_view.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"
#include "realtime/motion.hpp"

using kfc::engine::GameEngine;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::PieceState;
using kfc::model::Position;
using kfc::product::gameStateView;
using kfc::product::GameStateView;
using kfc::product::MoveLog;
using kfc::product::PlayerSnapshot;
using kfc::product::ScoreBoard;
using kfc::realtime::kSquareTravelMs;

namespace {

GameStateView viewOf(const GameEngine& engine) {
    return gameStateView(engine, ScoreBoard{}, MoveLog{});
}

const PlayerSnapshot& playerOf(const GameStateView& state, Color color) {
    for (const PlayerSnapshot& player : state.players) {
        if (player.color == color) return player;
    }
    FAIL("the state view carries no such player");
    return state.players.front();
}

}  // namespace

TEST_CASE("the state view carries board dimensions and the game-over flag") {
    GameEngine engine{Board{8, 8}};

    GameStateView state = viewOf(engine);

    CHECK(state.boardWidth == 8);
    CHECK(state.boardHeight == 8);
    CHECK_FALSE(state.gameOver);
    CHECK(state.pieces.empty());
}

TEST_CASE("the state view lists each piece in cell coordinates with its state") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    GameEngine engine{std::move(board)};

    GameStateView state = viewOf(engine);

    REQUIRE(state.pieces.size() == 1);
    const auto& piece = state.pieces.front();
    CHECK(piece.kind == PieceKind::kRook);
    CHECK(piece.color == Color::kWhite);
    CHECK(piece.cell == Position{4, 4});
    CHECK(piece.state == PieceState::kIdle);
    CHECK_FALSE(piece.movingTo.has_value());
    CHECK(piece.progress == doctest::Approx(0.0));
}

TEST_CASE("a moving piece exposes its destination and how far along it is") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    GameEngine engine{std::move(board)};
    engine.requestMove(Position{4, 4}, Position{4, 6});

    engine.advance(3 * kSquareTravelMs / 2);

    GameStateView state = viewOf(engine);
    REQUIRE(state.pieces.size() == 1);
    const auto& piece = state.pieces.front();
    CHECK(piece.state == PieceState::kMoving);
    CHECK(piece.cell == Position{4, 5});
    REQUIRE(piece.movingTo.has_value());
    CHECK(*piece.movingTo == Position{4, 6});
    CHECK(piece.progress == doctest::Approx(0.5));
    CHECK(piece.stateElapsedMs == kSquareTravelMs / 2);
}

// The jumper is off the board while an enemy holds its square, so the read-model
// has to reach past the board to keep the airborne piece present.
TEST_CASE("an airborne piece stays in the state view while an enemy takes its cell") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    board.addPiece(Piece{2, Color::kBlack, PieceKind::kRook, Position{4, 5}});
    GameEngine engine{std::move(board)};
    engine.requestMove(Position{4, 5}, Position{4, 4});
    engine.advance(kSquareTravelMs / 2);
    engine.requestJump(Position{4, 4});

    engine.advance(kSquareTravelMs / 2);

    GameStateView state = viewOf(engine);
    REQUIRE(state.pieces.size() == 2);
    const auto& airborne = state.pieces.back();
    CHECK(airborne.color == Color::kWhite);
    CHECK(airborne.state == PieceState::kAirborne);
    CHECK(state.pieces.front().color == Color::kBlack);
}

TEST_CASE("the state view carries both players, named and scoreless") {
    GameEngine engine{Board{8, 8}};

    GameStateView state = viewOf(engine);

    REQUIRE(state.players.size() == 2);
    CHECK(playerOf(state, Color::kWhite).name == "White");
    CHECK(playerOf(state, Color::kBlack).name == "Black");
    CHECK(playerOf(state, Color::kWhite).score == 0);
    CHECK(playerOf(state, Color::kBlack).moves.empty());
}

TEST_CASE("the state view carries the score a player has earned") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    board.addPiece(Piece{2, Color::kBlack, PieceKind::kQueen, Position{4, 6}});
    GameEngine engine{std::move(board)};
    ScoreBoard scores;
    MoveLog log;
    scores.subscribeTo(engine.events());
    log.subscribeTo(engine.events());

    engine.requestMove(Position{4, 4}, Position{4, 6});
    engine.advance(2 * kSquareTravelMs);
    GameStateView state = gameStateView(engine, scores, log);

    CHECK(playerOf(state, Color::kWhite).score == 9);
    CHECK(playerOf(state, Color::kBlack).score == 0);
}

// Unlike the view snapshot, the read-model keeps actions as raw records: it does
// not word them. That turn is the GUI's, not this layer's.
TEST_CASE("the state view keeps each logged action as a record for its player") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kPawn, Position{6, 4}});
    GameEngine engine{std::move(board)};
    ScoreBoard scores;
    MoveLog log;
    scores.subscribeTo(engine.events());
    log.subscribeTo(engine.events());

    engine.advance(2314);
    engine.requestMove(Position{6, 4}, Position{4, 4});
    GameStateView state = gameStateView(engine, scores, log);

    REQUIRE(playerOf(state, Color::kWhite).moves.size() == 1);
    const auto& record = playerOf(state, Color::kWhite).moves[0];
    CHECK(record.from == Position{6, 4});
    CHECK(record.to == Position{4, 4});
    CHECK(record.atMs == 2314);
    CHECK(playerOf(state, Color::kBlack).moves.empty());
}
