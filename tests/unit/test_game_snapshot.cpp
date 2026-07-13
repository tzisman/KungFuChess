#include <doctest/doctest.h>

#include <optional>

#include "engine/game_engine.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "view/game_snapshot.hpp"

using kfc::engine::GameEngine;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::PieceState;
using kfc::model::Position;
using kfc::view::buildSnapshot;
using kfc::view::GameSnapshot;

TEST_CASE("the view snapshot carries board dimensions and the game-over flag") {
    GameEngine engine{Board{8, 8}};

    GameSnapshot snapshot = buildSnapshot(engine.snapshot(), std::nullopt);

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

    GameSnapshot snapshot = buildSnapshot(engine.snapshot(), std::nullopt);

    REQUIRE(snapshot.pieces.size() == 1);
    const auto& piece = snapshot.pieces.front();
    CHECK(piece.kind == PieceKind::kRook);
    CHECK(piece.color == Color::kWhite);
    CHECK(piece.cell == Position{4, 4});
    CHECK(piece.state == PieceState::kIdle);
}

TEST_CASE("the view snapshot passes through the selected cell from the GUI") {
    GameEngine engine{Board{8, 8}};

    GameSnapshot snapshot = buildSnapshot(engine.snapshot(), Position{2, 3});

    REQUIRE(snapshot.selectedCell.has_value());
    CHECK(*snapshot.selectedCell == Position{2, 3});
}
