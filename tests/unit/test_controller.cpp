#include <doctest/doctest.h>

#include "engine/game_engine.hpp"
#include "input/board_mapper.hpp"
#include "input/controller.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"

using kfc::engine::GameEngine;
using kfc::input::BoardMapper;
using kfc::input::Controller;
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

int centerX(int col) { return col * kfc::input::kCellSize + kfc::input::kCellSize / 2; }
int centerY(int row) { return row * kfc::input::kCellSize + kfc::input::kCellSize / 2; }

}

TEST_CASE("a first click on an empty cell is ignored") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centerX(0), centerY(0));

    CHECK_FALSE(controller.selection().has_value());
}

TEST_CASE("a first click outside the board is ignored") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centerX(8), centerY(0));

    CHECK_FALSE(controller.selection().has_value());
}

TEST_CASE("a first click on a piece selects it") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centerX(4), centerY(4));

    REQUIRE(controller.selection().has_value());
    CHECK(*controller.selection() == Position{4, 4});
}

TEST_CASE("a second click inside the board requests the move and clears the selection") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centerX(4), centerY(4));
    controller.handleClick(centerX(7), centerY(4));

    CHECK_FALSE(controller.selection().has_value());

    engine.wait(3 * kSquareTravelMs);
    auto snapshot = engine.snapshot();
    CHECK_FALSE(snapshot.pieceAt(Position{4, 4}).has_value());
    CHECK(snapshot.pieceAt(Position{4, 7}).has_value());
}

TEST_CASE("a second click outside the board cancels the selection without moving") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centerX(4), centerY(4));
    controller.handleClick(centerX(8), centerY(4));

    CHECK_FALSE(controller.selection().has_value());

    engine.wait(3 * kSquareTravelMs);
    CHECK(engine.snapshot().pieceAt(Position{4, 4}).has_value());
}

TEST_CASE("an illegal second click still clears the selection and moves nothing") {
    GameEngine engine{boardWithRook()};
    Controller controller{engine, BoardMapper{8, 8}};

    controller.handleClick(centerX(4), centerY(4));
    controller.handleClick(centerX(5), centerY(5));

    CHECK_FALSE(controller.selection().has_value());

    engine.wait(3 * kSquareTravelMs);
    auto snapshot = engine.snapshot();
    CHECK(snapshot.pieceAt(Position{4, 4}).has_value());
    CHECK_FALSE(snapshot.pieceAt(Position{5, 5}).has_value());
}
