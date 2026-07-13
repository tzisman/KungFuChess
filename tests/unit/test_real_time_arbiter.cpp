#include <doctest/doctest.h>

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

    ArrivalReport report = arbiter.advance(kSquareTravelMs);  // 1000 of 3000ms

    CHECK_FALSE(report.pieceArrived);
    CHECK(arbiter.hasActiveMotion());
    CHECK(board.pieceAt(Position{4, 4}).has_value());
    CHECK_FALSE(board.pieceAt(Position{4, 7}).has_value());
}

TEST_CASE("the board updates when the piece arrives") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 7});

    ArrivalReport report = arbiter.advance(3 * kSquareTravelMs);

    CHECK(report.pieceArrived);
    CHECK_FALSE(report.captured.has_value());
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

    ArrivalReport report = arbiter.advance(2 * kSquareTravelMs);

    CHECK(report.pieceArrived);
    REQUIRE(report.captured.has_value());
    CHECK(report.captured->kind() == PieceKind::kPawn);
    CHECK_FALSE(report.kingCaptured);
    CHECK(board.pieceAt(Position{4, 6})->color() == Color::kWhite);
}

TEST_CASE("capturing the king is reported") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kKing, Position{4, 6}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 6});

    ArrivalReport report = arbiter.advance(2 * kSquareTravelMs);

    CHECK(report.kingCaptured);
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

    ArrivalReport report = arbiter.advance(kSquareTravelMs);

    CHECK_FALSE(report.pieceArrived);
}
