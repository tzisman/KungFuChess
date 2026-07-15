#include <doctest/doctest.h>

#include "engine/game_engine.hpp"
#include "input/board_mapper.hpp"
#include "input/controller.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"
#include "view/board_geometry.hpp"

using kfc::engine::GameEngine;
using kfc::input::BoardMapper;
using kfc::input::Controller;
using kfc::view::BoardGeometry;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::realtime::kSquareTravelMs;

namespace {

Board boardWithRook() {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    return board;
}

// These tests address the board in pixels, so they fix a cell size of their
// own rather than depending on whatever size a board happens to be drawn at.
constexpr int kCellSize = 100;

BoardMapper mapper() {
    return BoardMapper{BoardGeometry{8 * kCellSize, 8 * kCellSize, 8, 8}};
}

int centerX(int col) { return col * kCellSize + kCellSize / 2; }
int centerY(int row) { return row * kCellSize + kCellSize / 2; }

}

TEST_CASE("a first click on an empty cell is ignored") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, mapper()};

    controller.handleClick(centerX(0), centerY(0));

    CHECK_FALSE(controller.selection().has_value());
}

TEST_CASE("a first click outside the board is ignored") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, mapper()};

    controller.handleClick(centerX(8), centerY(0));

    CHECK_FALSE(controller.selection().has_value());
}

TEST_CASE("a first click on a piece selects it") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, mapper()};

    controller.handleClick(centerX(4), centerY(4));

    REQUIRE(controller.selection().has_value());
    CHECK(*controller.selection() == Position{4, 4});
}

TEST_CASE("a second click inside the board requests the move and clears the selection") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, mapper()};

    controller.handleClick(centerX(4), centerY(4));
    controller.handleClick(centerX(7), centerY(4));

    CHECK_FALSE(controller.selection().has_value());

    engine.wait(3 * kSquareTravelMs);
    const auto& board = engine.board();
    CHECK_FALSE(board.pieceAt(Position{4, 4}).has_value());
    CHECK(board.pieceAt(Position{4, 7}).has_value());
}

TEST_CASE("a second click outside the board cancels the selection without moving") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, mapper()};

    controller.handleClick(centerX(4), centerY(4));
    controller.handleClick(centerX(8), centerY(4));

    CHECK_FALSE(controller.selection().has_value());

    engine.wait(3 * kSquareTravelMs);
    CHECK(engine.board().pieceAt(Position{4, 4}).has_value());
}

TEST_CASE("an illegal second click still clears the selection and moves nothing") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, mapper()};

    controller.handleClick(centerX(4), centerY(4));
    controller.handleClick(centerX(5), centerY(5));

    CHECK_FALSE(controller.selection().has_value());

    engine.wait(3 * kSquareTravelMs);
    const auto& board = engine.board();
    CHECK(board.pieceAt(Position{4, 4}).has_value());
    CHECK_FALSE(board.pieceAt(Position{5, 5}).has_value());
}
