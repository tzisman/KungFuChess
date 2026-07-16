#include <doctest/doctest.h>

#include "engine/game_observer.hpp"
#include "io/move_notation.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "product/move_log.hpp"

using kfc::engine::ActionKind;
using kfc::io::clockText;
using kfc::io::notationOf;
using kfc::io::squareName;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::product::MoveRecord;

namespace {

constexpr int kBoardHeight = 8;

MoveRecord moveOf(PieceKind kind, Position from, Position to) {
    return {ActionKind::kMove, kind, from, to, 0};
}

}  // namespace

TEST_CASE("a square is named by its file and rank") {
    CHECK(squareName(Position{7, 0}, kBoardHeight) == "a1");
    CHECK(squareName(Position{0, 0}, kBoardHeight) == "a8");
    CHECK(squareName(Position{0, 7}, kBoardHeight) == "h8");
    CHECK(squareName(Position{4, 4}, kBoardHeight) == "e4");
}

TEST_CASE("ranks are counted from the board's own height, not a fixed eight") {
    CHECK(squareName(Position{0, 0}, 4) == "a4");
    CHECK(squareName(Position{3, 2}, 4) == "c1");
}

TEST_CASE("a piece is named by its letter and the squares it ran between") {
    CHECK(notationOf(moveOf(PieceKind::kKnight, Position{7, 1}, Position{5, 2}),
                     kBoardHeight) == "Nb1-c3");
    CHECK(notationOf(moveOf(PieceKind::kQueen, Position{7, 3}, Position{3, 3}),
                     kBoardHeight) == "Qd1-d5");
}

TEST_CASE("a pawn is named by its squares alone") {
    CHECK(notationOf(moveOf(PieceKind::kPawn, Position{6, 4}, Position{4, 4}),
                     kBoardHeight) == "e2-e4");
}

TEST_CASE("a jump is named where it stood, since it went nowhere") {
    MoveRecord jump{ActionKind::kJump, PieceKind::kRook, Position{4, 3},
                    Position{4, 3}, 0};

    CHECK(notationOf(jump, kBoardHeight) == "Jd4");
}

TEST_CASE("the clock reads as minutes, seconds and milliseconds") {
    CHECK(clockText(0) == "00:00.000");
    CHECK(clockText(2314) == "00:02.314");
    CHECK(clockText(64756) == "01:04.756");
    CHECK(clockText(600000) == "10:00.000");
}
