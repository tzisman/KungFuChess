#include <doctest/doctest.h>

#include <optional>
#include <utility>

#include "engine/game_engine.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"
#include "view/game_snapshot.hpp"

using kfc::engine::GameEngine;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::PieceState;
using kfc::model::Position;
using kfc::realtime::kSquareTravelMs;
using kfc::view::buildSnapshot;
using kfc::view::GameSnapshot;

TEST_CASE("the view snapshot carries board dimensions and the game-over flag") {
    GameEngine engine{Board{8, 8}};

    GameSnapshot snapshot = buildSnapshot(engine, std::nullopt);

    CHECK(snapshot.boardWidth == 8);
    CHECK(snapshot.boardHeight == 8);
    CHECK_FALSE(snapshot.gameOver);
    CHECK(snapshot.pieces.empty());
    CHECK_FALSE(snapshot.selectedCell.has_value());
}

TEST_CASE("the view snapshot lists each piece in cell coordinates with its state") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    GameEngine engine{std::move(board)};

    GameSnapshot snapshot = buildSnapshot(engine, std::nullopt);

    REQUIRE(snapshot.pieces.size() == 1);
    const auto& piece = snapshot.pieces.front();
    CHECK(piece.kind == PieceKind::kRook);
    CHECK(piece.color == Color::kWhite);
    CHECK(piece.cell == Position{4, 4});
    CHECK(piece.state == PieceState::kIdle);
    CHECK_FALSE(piece.movingTo.has_value());
    CHECK(piece.progress == doctest::Approx(0.0));
}

TEST_CASE("the view snapshot passes through the selected cell from the GUI") {
    GameEngine engine{Board{8, 8}};

    GameSnapshot snapshot = buildSnapshot(engine, Position{2, 3});

    REQUIRE(snapshot.selectedCell.has_value());
    CHECK(*snapshot.selectedCell == Position{2, 3});
}

// The jumper is off the board while an enemy holds its square, so the snapshot
// has to reach past the board to keep it on screen.
TEST_CASE("an airborne piece stays in the snapshot while an enemy takes its cell") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    board.addPiece(Piece{2, Color::kBlack, PieceKind::kRook, Position{4, 5}});
    GameEngine engine{std::move(board)};
    engine.requestMove(Position{4, 5}, Position{4, 4});
    engine.advance(kSquareTravelMs / 2);
    engine.requestJump(Position{4, 4});

    engine.advance(kSquareTravelMs / 2);

    GameSnapshot snapshot = buildSnapshot(engine, std::nullopt);
    REQUIRE(snapshot.pieces.size() == 2);
    const auto& airborne = snapshot.pieces.back();
    CHECK(airborne.color == Color::kWhite);
    CHECK(airborne.cell == Position{4, 4});
    CHECK(airborne.state == PieceState::kAirborne);
    CHECK(snapshot.pieces.front().color == Color::kBlack);
    CHECK(snapshot.pieces.front().cell == Position{4, 4});
}

TEST_CASE("a moving piece exposes its destination and how far along it is") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    GameEngine engine{std::move(board)};
    engine.requestMove(Position{4, 4}, Position{4, 6});

    engine.advance(kSquareTravelMs);

    GameSnapshot snapshot = buildSnapshot(engine, std::nullopt);
    REQUIRE(snapshot.pieces.size() == 1);
    const auto& piece = snapshot.pieces.front();
    CHECK(piece.state == PieceState::kMoving);
    REQUIRE(piece.movingTo.has_value());
    CHECK(*piece.movingTo == Position{4, 6});
    CHECK(piece.progress == doctest::Approx(0.5));
    CHECK(piece.stateElapsedMs == kSquareTravelMs);
}

TEST_CASE("the view snapshot carries the selected piece's move targets") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kKing, Position{4, 4}});
    GameEngine engine{std::move(board)};

    GameSnapshot snapshot = buildSnapshot(engine, Position{4, 4});

    CHECK(snapshot.moveTargets.size() == 8);
}

TEST_CASE("the view snapshot has no move targets when nothing is selected") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kKing, Position{4, 4}});
    GameEngine engine{std::move(board)};

    GameSnapshot snapshot = buildSnapshot(engine, std::nullopt);

    CHECK(snapshot.moveTargets.empty());
}
