#include <doctest/doctest.h>

#include <vector>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"
#include "realtime/real_time_arbiter.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::realtime::ArrivalReport;
using kfc::realtime::kSquareTravelMs;
using kfc::realtime::RealTimeArbiter;
using kfc::realtime::travelDurationMs;

namespace {

void place(Board& board, Color color, PieceKind kind, Position cell, int id) {
    board.addPiece(Piece{id, color, kind, cell});
}

}

TEST_CASE("a straight move takes one square-travel per cell") {
    CHECK(travelDurationMs(Position{4, 4}, Position{4, 7}) == 3 * kSquareTravelMs);
}

TEST_CASE("a diagonal move is measured in cell steps, not pixel distance") {
    CHECK(travelDurationMs(Position{0, 0}, Position{3, 3}) == 3 * kSquareTravelMs);
}

TEST_CASE("the board does not change before the piece arrives") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{4, 4}, Position{4, 7}));

    std::vector<ArrivalReport> reports = arbiter.advance(kSquareTravelMs);  // 1000 of 3000ms

    CHECK(reports.empty());
    CHECK(arbiter.hasActiveMotion());
    CHECK(board.pieceAt(Position{4, 4}).has_value());
    CHECK_FALSE(board.pieceAt(Position{4, 7}).has_value());
}

TEST_CASE("the board updates when the piece arrives") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 7});

    std::vector<ArrivalReport> reports = arbiter.advance(3 * kSquareTravelMs);

    REQUIRE(reports.size() == 1);
    CHECK_FALSE(reports[0].captured.has_value());
    CHECK_FALSE(arbiter.hasActiveMotion());
    CHECK_FALSE(board.pieceAt(Position{4, 4}).has_value());
    CHECK(board.pieceAt(Position{4, 7}).has_value());
}

TEST_CASE("arrival onto an enemy piece captures it") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{4, 6}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 6});

    std::vector<ArrivalReport> reports = arbiter.advance(2 * kSquareTravelMs);

    REQUIRE(reports.size() == 1);
    REQUIRE(reports[0].captured.has_value());
    CHECK(reports[0].captured->kind() == PieceKind::kPawn);
    CHECK_FALSE(reports[0].kingCaptured);
    CHECK(board.pieceAt(Position{4, 6})->color() == Color::kWhite);
}

TEST_CASE("capturing the king is reported") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kKing, Position{4, 6}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 6});

    std::vector<ArrivalReport> reports = arbiter.advance(2 * kSquareTravelMs);

    REQUIRE(reports.size() == 1);
    CHECK(reports[0].kingCaptured);
}

TEST_CASE("only one motion may be active at a time") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kWhite, PieceKind::kRook, Position{0, 0}, 2);
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{4, 4}, Position{4, 7}));
    CHECK_FALSE(arbiter.startMotion(Position{0, 0}, Position{0, 3}));
}

TEST_CASE("a new motion is allowed once the previous one arrives") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 7});
    arbiter.advance(3 * kSquareTravelMs);

    CHECK_FALSE(arbiter.hasActiveMotion());
    CHECK(arbiter.startMotion(Position{4, 7}, Position{4, 4}));
}

TEST_CASE("advancing with no active motion reports nothing") {
    Board board{8, 8};
    RealTimeArbiter arbiter{board};

    std::vector<ArrivalReport> reports = arbiter.advance(kSquareTravelMs);

    CHECK(reports.empty());
}

TEST_CASE("an airborne piece lands as idle after the jump window") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startJump(Position{4, 4}));
    CHECK(board.pieceAt(Position{4, 4})->state() ==
          kfc::model::PieceState::kAirborne);

    arbiter.advance(kfc::realtime::kJumpDurationMs);

    CHECK(board.pieceAt(Position{4, 4})->state() ==
          kfc::model::PieceState::kIdle);
}

TEST_CASE("an enemy arriving mid-jump sits on the cell, airborne piece lifted") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{4, 5}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 5}, Position{4, 4});
    arbiter.advance(kSquareTravelMs / 2);  // enemy halfway, no jump yet

    arbiter.startJump(Position{4, 4});
    arbiter.advance(kSquareTravelMs / 2);  // enemy arrives; jump still airborne

    CHECK(board.pieceAt(Position{4, 4})->color() == Color::kBlack);
    CHECK(board.pieceAt(Position{4, 4})->kind() == PieceKind::kPawn);
}

TEST_CASE("the airborne piece captures the arrived enemy when it lands") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{4, 5}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startJump(Position{4, 4});
    arbiter.startMotion(Position{4, 5}, Position{4, 4});

    std::vector<ArrivalReport> reports = arbiter.advance(kfc::realtime::kJumpDurationMs);

    const ArrivalReport& landing = reports.back();
    CHECK_FALSE(landing.landed);
    REQUIRE(landing.captured.has_value());
    CHECK(landing.captured->kind() == PieceKind::kPawn);
    CHECK(board.pieceAt(Position{4, 4})->color() == Color::kWhite);
    CHECK(board.pieceAt(Position{4, 4})->kind() == PieceKind::kRook);
}

TEST_CASE("capturing an arrived king on landing is reported") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kKing, Position{4, 5}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startJump(Position{4, 4});
    arbiter.startMotion(Position{4, 5}, Position{4, 4});

    std::vector<ArrivalReport> reports = arbiter.advance(kfc::realtime::kJumpDurationMs);

    CHECK(reports.back().kingCaptured);
    CHECK_FALSE(reports.back().landed);
}
