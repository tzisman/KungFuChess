#include <doctest/doctest.h>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "rules/rule_engine.hpp"

using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::rules::MoveValidation;
using kfc::rules::Reason;
using kfc::rules::reasonCode;
using kfc::rules::RuleEngine;

namespace {

void place(Board& board, Color color, PieceKind kind, Position cell, int id) {
    board.addPiece(Piece{id, color, kind, cell});
}

MoveValidation check(const Board& board, Position from, Position to) {
    return RuleEngine{}.validate(board, from, to);
}

}

TEST_CASE("a legal rook move onto an empty cell is valid") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);

    MoveValidation result = check(board, Position{4, 4}, Position{4, 7});

    CHECK(result.isValid);
    CHECK(result.reason == Reason::kOk);
}

TEST_CASE("capturing an enemy piece is valid") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kBlack, PieceKind::kPawn, Position{4, 6}, 2);

    MoveValidation result = check(board, Position{4, 4}, Position{4, 6});

    CHECK(result.isValid);
    CHECK(result.reason == Reason::kOk);
}

TEST_CASE("a destination outside the board is rejected") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);

    MoveValidation result = check(board, Position{4, 4}, Position{4, 8});

    CHECK_FALSE(result.isValid);
    CHECK(result.reason == Reason::kOutsideBoard);
}

TEST_CASE("a move from an empty source is rejected") {
    Board board{8, 8};

    MoveValidation result = check(board, Position{4, 4}, Position{4, 5});

    CHECK_FALSE(result.isValid);
    CHECK(result.reason == Reason::kEmptySource);
}

TEST_CASE("a move onto a friendly piece is rejected") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);
    place(board, Color::kWhite, PieceKind::kPawn, Position{4, 6}, 2);

    MoveValidation result = check(board, Position{4, 4}, Position{4, 6});

    CHECK_FALSE(result.isValid);
    CHECK(result.reason == Reason::kFriendlyDestination);
}

TEST_CASE("a move the piece rule forbids is rejected") {
    Board board{8, 8};
    place(board, Color::kWhite, PieceKind::kRook, Position{4, 4}, 1);

    MoveValidation result = check(board, Position{4, 4}, Position{6, 6});

    CHECK_FALSE(result.isValid);
    CHECK(result.reason == Reason::kIllegalPieceMove);
}

TEST_CASE("reasons map to stable machine-readable codes") {
    CHECK(reasonCode(Reason::kOk) == "ok");
    CHECK(reasonCode(Reason::kOutsideBoard) == "outside_board");
    CHECK(reasonCode(Reason::kEmptySource) == "empty_source");
    CHECK(reasonCode(Reason::kFriendlyDestination) == "friendly_destination");
    CHECK(reasonCode(Reason::kIllegalPieceMove) == "illegal_piece_move");
}
