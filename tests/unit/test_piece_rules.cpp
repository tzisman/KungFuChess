#include <doctest/doctest.h>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "rules/piece_rules.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::rules::Destinations;
using kfc::rules::promotedKind;
using kfc::rules::ruleFor;

namespace {

Piece pieceAt(Color color, PieceKind kind, Position cell) {
    return Piece{1, color, kind, cell};
}

void place(Board& board, Color color, PieceKind kind, Position cell, int id) {
    board.addPiece(Piece{id, color, kind, cell});
}

bool reaches(const Destinations& destinations, Position cell) {
    return destinations.count(cell) != 0;
}

Destinations legalFor(const Board& board, const Piece& piece) {
    return ruleFor(piece.kind()).legalDestinations(board, piece);
}

}

TEST_CASE("rook moves across an empty row and column") {
    Board board{8, 8};
    Piece rook = pieceAt(Color::kWhite, PieceKind::kRook, Position{4, 4});
    Destinations moves = legalFor(board, rook);

    CHECK(reaches(moves, Position{4, 0}));
    CHECK(reaches(moves, Position{4, 7}));
    CHECK(reaches(moves, Position{0, 4}));
    CHECK(reaches(moves, Position{7, 4}));
    CHECK_FALSE(reaches(moves, Position{5, 5}));
}

TEST_CASE("rook stops before a friendly blocker") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kWhite, PieceKind::kPawn, Position{4, 6}, 2);
    Piece rook = *board.pieceAt(Position{4, 4});
    Destinations moves = legalFor(board, rook);

    CHECK(reaches(moves, Position{4, 5}));
    CHECK_FALSE(reaches(moves, Position{4, 6}));
    CHECK_FALSE(reaches(moves, Position{4, 7}));
}

TEST_CASE("rook captures an enemy blocker but does not pass it") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{4, 6}, 2);
    Piece rook = *board.pieceAt(Position{4, 4});
    Destinations moves = legalFor(board, rook);

    CHECK(reaches(moves, Position{4, 5}));
    CHECK(reaches(moves, Position{4, 6}));
    CHECK_FALSE(reaches(moves, Position{4, 7}));
}

TEST_CASE("bishop moves diagonally and not straight") {
    Board board{8, 8};
    Piece bishop = pieceAt(Color::kWhite, PieceKind::kBishop, Position{4, 4});
    Destinations moves = legalFor(board, bishop);

    CHECK(reaches(moves, Position{5, 5}));
    CHECK(reaches(moves, Position{3, 3}));
    CHECK(reaches(moves, Position{6, 2}));
    CHECK_FALSE(reaches(moves, Position{4, 5}));
    CHECK_FALSE(reaches(moves, Position{5, 4}));
}

TEST_CASE("queen combines rook and bishop movement") {
    Board board{8, 8};
    Piece queen = pieceAt(Color::kWhite, PieceKind::kQueen, Position{4, 4});
    Destinations moves = legalFor(board, queen);

    CHECK(reaches(moves, Position{4, 0}));
    CHECK(reaches(moves, Position{0, 4}));
    CHECK(reaches(moves, Position{7, 7}));
    CHECK(reaches(moves, Position{1, 1}));
}

TEST_CASE("knight jumps over blockers") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kKnight, Position{4, 4}, 1);
    place(board, Color::kWhite, PieceKind::kPawn, Position{4, 5}, 2);
    place(board, Color::kWhite, PieceKind::kPawn, Position{5, 4}, 3);
    Piece knight = *board.pieceAt(Position{4, 4});
    Destinations moves = legalFor(board, knight);

    CHECK(reaches(moves, Position{6, 5}));
    CHECK(reaches(moves, Position{5, 6}));
    CHECK(reaches(moves, Position{2, 3}));
    CHECK_FALSE(reaches(moves, Position{4, 5}));
}

TEST_CASE("king moves one cell only") {
    Board board{8, 8};
    Piece king = pieceAt(Color::kWhite, PieceKind::kKing, Position{4, 4});
    Destinations moves = legalFor(board, king);

    CHECK(reaches(moves, Position{5, 4}));
    CHECK(reaches(moves, Position{5, 5}));
    CHECK(reaches(moves, Position{3, 4}));
    CHECK_FALSE(reaches(moves, Position{6, 4}));
    CHECK_FALSE(reaches(moves, Position{4, 6}));
}

TEST_CASE("pawn advances one step forward into an empty cell") {
    Board board{8, 8};
    Piece pawn = pieceAt(Color::kWhite, PieceKind::kPawn, Position{3, 4});
    Destinations moves = legalFor(board, pawn);

    CHECK(reaches(moves, Position{2, 4}));
    CHECK_FALSE(reaches(moves, Position{1, 4}));
    CHECK_FALSE(reaches(moves, Position{4, 4}));
}

TEST_CASE("pawn takes a double step only from its starting row") {
    Board board{8, 8};
    Piece pawn = pieceAt(Color::kWhite, PieceKind::kPawn, Position{6, 4});
    Destinations moves = legalFor(board, pawn);

    CHECK(reaches(moves, Position{5, 4}));
    CHECK(reaches(moves, Position{4, 4}));
}

TEST_CASE("pawn cannot advance forward onto an occupied cell") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kPawn, Position{6, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{5, 4}, 2);
    Piece pawn = *board.pieceAt(Position{6, 4});
    Destinations moves = legalFor(board, pawn);

    CHECK_FALSE(reaches(moves, Position{5, 4}));
    CHECK_FALSE(reaches(moves, Position{4, 4}));
}

TEST_CASE("pawn captures one diagonal step forward") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kPawn, Position{3, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{2, 5}, 2);
    place(board, Color::kWhite, PieceKind::kPawn, Position{2, 3}, 3);
    Piece pawn = *board.pieceAt(Position{3, 4});
    Destinations moves = legalFor(board, pawn);

    CHECK(reaches(moves, Position{2, 5}));
    CHECK_FALSE(reaches(moves, Position{2, 3}));
}

TEST_CASE("black pawn moves and captures downward") {
    Board board{8, 8};
    place(board, Color::kBlack, PieceKind::kPawn, Position{1, 4}, 1);
    place(board, Color::kWhite, PieceKind::kPawn, Position{2, 5}, 2);
    Piece pawn = *board.pieceAt(Position{1, 4});
    Destinations moves = legalFor(board, pawn);

    CHECK(reaches(moves, Position{2, 4}));
    CHECK(reaches(moves, Position{3, 4}));
    CHECK(reaches(moves, Position{2, 5}));
}

TEST_CASE("a white pawn on the last row promotes to a queen") {
    Board board{8, 8};
    Piece pawn = pieceAt(Color::kWhite, PieceKind::kPawn, Position{0, 4});
    CHECK(promotedKind(board, pawn) == PieceKind::kQueen);
}

TEST_CASE("a black pawn on the last row promotes to a queen") {
    Board board{8, 8};
    Piece pawn = pieceAt(Color::kBlack, PieceKind::kPawn, Position{7, 4});
    CHECK(promotedKind(board, pawn) == PieceKind::kQueen);
}

TEST_CASE("a pawn short of the last row does not promote") {
    Board board{8, 8};
    Piece pawn = pieceAt(Color::kWhite, PieceKind::kPawn, Position{6, 4});
    CHECK_FALSE(promotedKind(board, pawn).has_value());
}

TEST_CASE("a non-pawn on the last row does not promote") {
    Board board{8, 8};
    Piece rook = pieceAt(Color::kWhite, PieceKind::kRook, Position{7, 4});
    CHECK_FALSE(promotedKind(board, rook).has_value());
}
