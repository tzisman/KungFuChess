#include <doctest/doctest.h>

#include <sstream>
#include <utility>

#include "engine/game_engine.hpp"
#include "io/board_printer.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

using kfc::engine::GameEngine;
using kfc::io::printBoard;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;

TEST_CASE("prints an empty board as dots") {
    GameEngine engine{Board{3, 2}};
    std::ostringstream out;

    printBoard(engine.snapshot(), out);

    CHECK(out.str() == ". . .\n. . .\n");
}

TEST_CASE("prints pieces as color-and-kind tokens") {
    Board board{3, 3};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kKing, Position{0, 0}});
    board.addPiece(Piece{2, Color::kBlack, PieceKind::kQueen, Position{2, 2}});
    GameEngine engine{std::move(board)};
    std::ostringstream out;

    printBoard(engine.snapshot(), out);

    CHECK(out.str() == "wK . .\n. . .\n. . bQ\n");
}
