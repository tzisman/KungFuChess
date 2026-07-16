#include <doctest/doctest.h>

#include "model/piece.hpp"
#include "model/piece_cost.hpp"

using kfc::model::costOf;
using kfc::model::PieceKind;

TEST_CASE("a piece costs what it is worth to take") {
    CHECK(costOf(PieceKind::kQueen) == 9);
    CHECK(costOf(PieceKind::kRook) == 5);
    CHECK(costOf(PieceKind::kBishop) == 3);
    CHECK(costOf(PieceKind::kKnight) == 3);
    CHECK(costOf(PieceKind::kPawn) == 1);
}

TEST_CASE("the king is worth no material, as taking it wins outright") {
    CHECK(costOf(PieceKind::kKing) == 0);
}

TEST_CASE("the pieces rank against each other as chess has them") {
    CHECK(costOf(PieceKind::kQueen) > costOf(PieceKind::kRook));
    CHECK(costOf(PieceKind::kRook) > costOf(PieceKind::kBishop));
    CHECK(costOf(PieceKind::kBishop) == costOf(PieceKind::kKnight));
    CHECK(costOf(PieceKind::kKnight) > costOf(PieceKind::kPawn));
}
