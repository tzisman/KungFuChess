#include <doctest/doctest.h>

#include "engine/game_engine.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"

using kfc::engine::GameEngine;
using kfc::engine::MoveResult;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::realtime::kLongRestMs;
using kfc::realtime::kSquareTravelMs;

namespace {

Board boardWith(std::initializer_list<Piece> pieces) {
    Board board{8, 8};
    for (const Piece& piece : pieces) {
        board.addPiece(piece);
    }
    return board;
}

}

TEST_CASE("a legal move is accepted and reported as ok") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};

    MoveResult result = engine.requestMove(Position{4, 4}, Position{4, 7});

    CHECK(result.isAccepted);
    CHECK(result.reason == "ok");
}

TEST_CASE("an illegal piece move carries the rule-level reason") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};

    MoveResult result = engine.requestMove(Position{4, 4}, Position{5, 5});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "illegal_piece_move");
}

TEST_CASE("moving from an empty source carries the rule-level reason") {
    GameEngine engine{Board{8, 8}};

    MoveResult result = engine.requestMove(Position{0, 0}, Position{0, 3});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "empty_source");
}

TEST_CASE("a second move is rejected while a motion is in progress") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kWhite, PieceKind::kRook, Position{0, 0}},
    })};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    MoveResult result = engine.requestMove(Position{0, 0}, Position{0, 3});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "motion_in_progress");
}

TEST_CASE("a piece that just arrived is resting and cannot move yet") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    engine.advance(3 * kSquareTravelMs);

    MoveResult result = engine.requestMove(Position{4, 7}, Position{4, 4});
    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "not_idle");
}

TEST_CASE("a new move is allowed once the cooldown after arrival elapses") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    engine.advance(3 * kSquareTravelMs);
    engine.advance(kLongRestMs);

    CHECK(engine.requestMove(Position{4, 7}, Position{4, 4}).isAccepted);
}

TEST_CASE("wait advances the board through the arbiter") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    engine.advance(3 * kSquareTravelMs);

    CHECK_FALSE(engine.board().pieceAt(Position{4, 4}).has_value());
    CHECK(engine.board().pieceAt(Position{4, 7}).has_value());
}

TEST_CASE("capturing the king ends the game") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 6}},
    })};
    engine.requestMove(Position{4, 4}, Position{4, 6});

    engine.advance(2 * kSquareTravelMs);

    CHECK(engine.isOver());
}

TEST_CASE("moves are rejected once the game is over") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 6}},
    })};
    engine.requestMove(Position{4, 4}, Position{4, 6});
    engine.advance(2 * kSquareTravelMs);

    MoveResult result = engine.requestMove(Position{4, 6}, Position{4, 5});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "game_over");
}

TEST_CASE("a pawn reaching the last row is promoted to a queen") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kPawn, Position{1, 4}}})};
    engine.requestMove(Position{1, 4}, Position{0, 4});

    engine.advance(kSquareTravelMs);

    auto piece = engine.board().pieceAt(Position{0, 4});
    REQUIRE(piece.has_value());
    CHECK(piece->kind() == PieceKind::kQueen);
}

TEST_CASE("a jump on an idle piece is accepted") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK(result.isAccepted);
    CHECK(result.reason == "ok");
}

TEST_CASE("a jump on an empty cell is rejected") {
    GameEngine engine{Board{8, 8}};

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "no_piece");
}

TEST_CASE("a moving piece cannot jump") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "not_idle");
}

TEST_CASE("an airborne piece cannot jump again") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestJump(Position{4, 4});

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "not_idle");
}

TEST_CASE("an airborne piece capturing an arriving king ends the game") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 5}},
    })};
    engine.requestJump(Position{4, 4});
    engine.requestMove(Position{4, 5}, Position{4, 4});

    engine.advance(kSquareTravelMs);

    CHECK(engine.isOver());
    CHECK(engine.board().pieceAt(Position{4, 4})->kind() == PieceKind::kRook);
}
