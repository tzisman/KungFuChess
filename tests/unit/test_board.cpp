#include <doctest/doctest.h>

#include "model/board.hpp"
#include "model/errors.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

using kfc::model::Board;
using kfc::model::CellEmptyError;
using kfc::model::CellOccupiedError;
using kfc::model::Color;
using kfc::model::OutOfBoundsError;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;

namespace {
Piece pawnAt(Position cell, int id = 1) {
    return Piece{id, Color::kWhite, PieceKind::kPawn, cell};
}
}

TEST_CASE("a board reports the width and height it was built with") {
    Board board{8, 6};
    CHECK(board.width() == 8);
    CHECK(board.height() == 6);
}

TEST_CASE("cells inside the board are in bounds and cells outside are not") {
    Board board{8, 8};
    CHECK(board.inBounds(Position{0, 0}));
    CHECK(board.inBounds(Position{7, 7}));
    CHECK_FALSE(board.inBounds(Position{-1, 0}));
    CHECK_FALSE(board.inBounds(Position{0, 8}));
    CHECK_FALSE(board.inBounds(Position{8, 0}));
}

TEST_CASE("an empty cell holds no piece") {
    Board board{8, 8};
    CHECK_FALSE(board.pieceAt(Position{4, 4}).has_value());
}

TEST_CASE("an occupied cell returns the piece that was placed there") {
    Board board{8, 8};
    board.addPiece(pawnAt(Position{4, 4}, 7));
    auto piece = board.pieceAt(Position{4, 4});
    REQUIRE(piece.has_value());
    CHECK(piece->id() == 7);
    CHECK(piece->cell() == Position{4, 4});
}

TEST_CASE("placing two pieces on the same cell is rejected") {
    Board board{8, 8};
    board.addPiece(pawnAt(Position{4, 4}, 1));
    CHECK_THROWS_AS(board.addPiece(pawnAt(Position{4, 4}, 2)), CellOccupiedError);
}

TEST_CASE("placing a piece outside the board is rejected") {
    Board board{8, 8};
    CHECK_THROWS_AS(board.addPiece(pawnAt(Position{8, 0})), OutOfBoundsError);
}

TEST_CASE("moving a piece clears the source and fills the destination") {
    Board board{8, 8};
    board.addPiece(pawnAt(Position{1, 0}, 3));
    board.movePiece(Position{1, 0}, Position{3, 0});
    CHECK_FALSE(board.pieceAt(Position{1, 0}).has_value());
    auto moved = board.pieceAt(Position{3, 0});
    REQUIRE(moved.has_value());
    CHECK(moved->id() == 3);
    CHECK(moved->cell() == Position{3, 0});
}

TEST_CASE("moving onto an occupied cell is rejected") {
    Board board{8, 8};
    board.addPiece(pawnAt(Position{1, 0}, 1));
    board.addPiece(pawnAt(Position{3, 0}, 2));
    CHECK_THROWS_AS(board.movePiece(Position{1, 0}, Position{3, 0}), CellOccupiedError);
}

TEST_CASE("moving from an empty cell is rejected") {
    Board board{8, 8};
    CHECK_THROWS_AS(board.movePiece(Position{1, 0}, Position{3, 0}), CellEmptyError);
}

TEST_CASE("removing a captured piece clears its cell") {
    Board board{8, 8};
    board.addPiece(pawnAt(Position{2, 2}, 5));
    board.removePiece(Position{2, 2});
    CHECK_FALSE(board.pieceAt(Position{2, 2}).has_value());
}

TEST_CASE("removing from an empty cell is rejected") {
    Board board{8, 8};
    CHECK_THROWS_AS(board.removePiece(Position{2, 2}), CellEmptyError);
}

TEST_CASE("setting a piece kind changes only its kind") {
    Board board{8, 8};
    board.addPiece(pawnAt(Position{7, 0}, 4));
    board.setPieceKind(Position{7, 0}, PieceKind::kQueen);
    auto piece = board.pieceAt(Position{7, 0});
    REQUIRE(piece.has_value());
    CHECK(piece->kind() == PieceKind::kQueen);
    CHECK(piece->id() == 4);
    CHECK(piece->cell() == Position{7, 0});
}

TEST_CASE("setting a piece kind on an empty cell is rejected") {
    Board board{8, 8};
    CHECK_THROWS_AS(board.setPieceKind(Position{7, 0}, PieceKind::kQueen), CellEmptyError);
}
