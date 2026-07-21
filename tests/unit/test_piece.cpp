#include <doctest/doctest.h>

#include <sstream>
#include <string>

#include "model/piece.hpp"

using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::PieceState;
using kfc::model::Position;

TEST_CASE("a piece exposes the identity it was created with") {
    Piece piece{7, Color::kWhite, PieceKind::kKnight, Position{0, 1}};
    CHECK(piece.id() == 7);
    CHECK(piece.color() == Color::kWhite);
    CHECK(piece.kind() == PieceKind::kKnight);
    CHECK(piece.cell() == Position{0, 1});
}

TEST_CASE("a piece starts idle by default") {
    Piece piece{1, Color::kBlack, PieceKind::kPawn, Position{6, 4}};
    CHECK(piece.state() == PieceState::kIdle);
}

TEST_CASE("a piece can be relocated without changing its identity") {
    Piece piece{1, Color::kBlack, PieceKind::kPawn, Position{6, 4}};
    piece.setCell(Position{5, 4});
    CHECK(piece.cell() == Position{5, 4});
    CHECK(piece.id() == 1);
    CHECK(piece.kind() == PieceKind::kPawn);
    CHECK(piece.color() == Color::kBlack);
}

TEST_CASE("a piece moves through its lifecycle states") {
    Piece piece{1, Color::kWhite, PieceKind::kQueen, Position{0, 3}};
    piece.setState(PieceState::kMoving);
    CHECK(piece.state() == PieceState::kMoving);
    piece.setState(PieceState::kCaptured);
    CHECK(piece.state() == PieceState::kCaptured);
}

TEST_CASE("a piece has a readable representation for assertion failures") {
    std::ostringstream os;
    os << Piece{7, Color::kWhite, PieceKind::kKnight, Position{0, 1}};
    CHECK(os.str() ==
          "Piece(id=7, color=White, kind=Knight, "
          "cell=Position(row=0, col=1), state=Idle)");
}

TEST_CASE("a colour has a name") {
    CHECK(std::string{kfc::model::nameOf(Color::kWhite)} == "White");
    CHECK(std::string{kfc::model::nameOf(Color::kBlack)} == "Black");
}

// The name and the printed form are one definition, so a colour cannot be
// called one thing on screen and another in a diagnostic.
TEST_CASE("a printed colour reads as the name it is given") {
    std::ostringstream os;
    os << Color::kBlack;
    CHECK(os.str() == kfc::model::nameOf(Color::kBlack));
}

TEST_CASE("the opposite of a colour is the other one") {
    CHECK(kfc::model::opposite(Color::kWhite) == Color::kBlack);
    CHECK(kfc::model::opposite(Color::kBlack) == Color::kWhite);
}
