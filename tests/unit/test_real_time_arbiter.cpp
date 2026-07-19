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
using kfc::model::PieceState;
using kfc::realtime::ArrivalReport;
using kfc::realtime::buildRoute;
using kfc::realtime::CellProgress;
using kfc::realtime::kJumpDurationMs;
using kfc::realtime::kLongRestMs;
using kfc::realtime::kShortRestMs;
using kfc::realtime::kSquareTravelMs;
using kfc::realtime::Motion;
using kfc::realtime::MotionProfiles;
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

TEST_CASE("the board does not change mid-hop, before the piece reaches the next cell") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{4, 4}, Position{4, 7}));

    std::vector<ArrivalReport> reports = arbiter.advance(kSquareTravelMs / 2);

    CHECK(reports.empty());
    CHECK(arbiter.hasActiveMotion());
    CHECK(board.pieceAt(Position{4, 4}).has_value());
    CHECK_FALSE(board.pieceAt(Position{4, 5}).has_value());
}

TEST_CASE("a sliding piece steps through the cells along its route") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 7});

    CHECK(arbiter.advance(kSquareTravelMs).empty());
    CHECK(board.pieceAt(Position{4, 5}).has_value());
    CHECK_FALSE(board.pieceAt(Position{4, 4}).has_value());

    CHECK(arbiter.advance(kSquareTravelMs).empty());
    CHECK(board.pieceAt(Position{4, 6}).has_value());

    REQUIRE(arbiter.advance(kSquareTravelMs).size() == 1);
    CHECK(board.pieceAt(Position{4, 7}).has_value());
    CHECK_FALSE(arbiter.hasActiveMotion());
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

TEST_CASE("different pieces may be in motion at the same time") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kWhite, PieceKind::kRook, Position{0, 0}, 2);
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{4, 4}, Position{4, 7}));
    CHECK(arbiter.startMotion(Position{0, 0}, Position{0, 3}));
}

TEST_CASE("a piece already in motion cannot be commanded again") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startMotion(Position{4, 4}, Position{4, 7}));
    CHECK_FALSE(arbiter.startMotion(Position{4, 4}, Position{4, 6}));
}

TEST_CASE("starting a motion from an empty cell is refused") {
    Board board{8, 8};
    RealTimeArbiter arbiter{board};

    CHECK_FALSE(arbiter.startMotion(Position{4, 4}, Position{4, 7}));
}

TEST_CASE("a motion carries the identity of the piece that started it") {
    Motion motion{7, Position{4, 4},
                  buildRoute(Position{4, 4}, Position{4, 7}, kSquareTravelMs)};

    CHECK(motion.pieceId() == 7);
    CHECK(motion.currentCell() == Position{4, 4});
    CHECK(motion.destination() == Position{4, 7});
    CHECK(motion.durationMs() == kSquareTravelMs);
}

TEST_CASE("a slower profile stretches the travel time of that kind") {
    MotionProfiles profiles;
    profiles.setTiming(PieceKind::kRook, 500, 200);

    CHECK(profiles.squareTravelMs(PieceKind::kRook) == 500);
    CHECK(profiles.jumpDurationMs(PieceKind::kRook) == 200);
    CHECK(profiles.squareTravelMs(PieceKind::kPawn) == kSquareTravelMs);
    CHECK(profiles.jumpDurationMs(PieceKind::kPawn) == kJumpDurationMs);
}

TEST_CASE("the arbiter times a motion by the moving piece's profile") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    MotionProfiles profiles;
    profiles.setTiming(PieceKind::kRook, 500, 200);
    RealTimeArbiter arbiter{board, profiles};
    arbiter.startMotion(Position{4, 4}, Position{4, 7});

    CHECK(arbiter.advance(3 * 500 - 1).empty());
    CHECK(arbiter.advance(1).size() == 1);
    CHECK(board.pieceAt(Position{4, 7}).has_value());
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

TEST_CASE("an airborne piece rests briefly after the jump, then goes idle") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};

    CHECK(arbiter.startJump(Position{4, 4}));
    CHECK(board.pieceAt(Position{4, 4})->state() == PieceState::kAirborne);

    arbiter.advance(kJumpDurationMs);
    CHECK(board.pieceAt(Position{4, 4})->state() == PieceState::kShortResting);

    arbiter.advance(kShortRestMs);
    CHECK(board.pieceAt(Position{4, 4})->state() == PieceState::kIdle);
}

TEST_CASE("an arrived piece takes the long rest, then goes idle") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 7});

    arbiter.advance(3 * kSquareTravelMs);
    CHECK(board.pieceAt(Position{4, 7})->state() == PieceState::kLongResting);

    arbiter.advance(kShortRestMs);
    CHECK(board.pieceAt(Position{4, 7})->state() == PieceState::kLongResting);

    arbiter.advance(kLongRestMs - kShortRestMs);
    CHECK(board.pieceAt(Position{4, 7})->state() == PieceState::kIdle);
}

TEST_CASE("capturing a resting piece leaves the capturer resting") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kRook, Position{0, 7}, 2);
    RealTimeArbiter arbiter{board};

    arbiter.startMotion(Position{4, 4}, Position{4, 7});
    arbiter.advance(3 * kSquareTravelMs);  // white arrives at 4,7 and rests

    arbiter.startMotion(Position{0, 7}, Position{4, 7});
    arbiter.advance(4 * kSquareTravelMs);  // black captures the resting white

    CHECK(board.pieceAt(Position{4, 7})->id() == 2);
    CHECK(board.pieceAt(Position{4, 7})->state() == PieceState::kLongResting);
}

TEST_CASE("progress mid-motion reports the next cell and the fraction covered") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 4}, Position{4, 7});

    arbiter.advance(3 * kSquareTravelMs / 2);

    CellProgress progress = arbiter.progressAt(Position{4, 5});
    REQUIRE(progress.movingTo.has_value());
    CHECK(*progress.movingTo == Position{4, 6});
    CHECK(progress.progress == doctest::Approx(0.5));
    CHECK(progress.stateElapsedMs == kSquareTravelMs / 2);
}

TEST_CASE("progress of a resting piece has no destination but counts its rest") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};
    arbiter.startJump(Position{4, 4});
    arbiter.advance(kJumpDurationMs);
    arbiter.advance(kShortRestMs / 3);

    CellProgress progress = arbiter.progressAt(Position{4, 4});
    CHECK_FALSE(progress.movingTo.has_value());
    CHECK(progress.stateElapsedMs == kShortRestMs / 3);
    CHECK(progress.progress == doctest::Approx(1.0 / 3));
}

TEST_CASE("progress of an idle cell is empty") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    RealTimeArbiter arbiter{board};

    CellProgress progress = arbiter.progressAt(Position{4, 4});
    CHECK_FALSE(progress.movingTo.has_value());
    CHECK(progress.progress == doctest::Approx(0.0));
    CHECK(progress.stateElapsedMs == 0);
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

TEST_CASE("a lifted piece is still reported while it is off the board") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{4, 5}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startMotion(Position{4, 5}, Position{4, 4});
    arbiter.advance(kSquareTravelMs / 2);

    arbiter.startJump(Position{4, 4});
    CHECK(arbiter.liftedPieces().empty());  // nothing has taken its square yet

    arbiter.advance(kSquareTravelMs / 2);

    std::vector<kfc::realtime::LiftedPiece> lifted = arbiter.liftedPieces();
    REQUIRE(lifted.size() == 1);
    CHECK(lifted.front().piece.color() == Color::kWhite);
    CHECK(lifted.front().piece.kind() == PieceKind::kRook);
    CHECK(lifted.front().piece.cell() == Position{4, 4});
    CHECK(lifted.front().piece.state() == PieceState::kAirborne);
}

TEST_CASE("a piece is no longer reported as lifted once it lands") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{4, 5}, 2);
    RealTimeArbiter arbiter{board};
    arbiter.startJump(Position{4, 4});
    arbiter.startMotion(Position{4, 5}, Position{4, 4});
    arbiter.advance(kJumpDurationMs);

    CHECK(arbiter.liftedPieces().empty());
    CHECK(board.pieceAt(Position{4, 4})->kind() == PieceKind::kRook);
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
