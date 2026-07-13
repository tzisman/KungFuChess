#include <doctest/doctest.h>

#include <sstream>

#include "io/board_parser.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

using kfc::io::ParseError;
using kfc::io::parseInput;
using kfc::io::ParsedInput;
using kfc::model::Color;
using kfc::model::PieceKind;
using kfc::model::Position;

TEST_CASE("parses board dimensions and pieces from the board section") {
    std::istringstream in(
        "Board:\n"
        "wK . .\n"
        ". . .\n"
        ". . bQ\n"
        "Commands:\n"
        "print board\n");

    ParsedInput parsed = parseInput(in);

    CHECK(parsed.board.width() == 3);
    CHECK(parsed.board.height() == 3);

    auto king = parsed.board.pieceAt(Position{0, 0});
    REQUIRE(king.has_value());
    CHECK(king->color() == Color::kWhite);
    CHECK(king->kind() == PieceKind::kKing);

    auto queen = parsed.board.pieceAt(Position{2, 2});
    REQUIRE(queen.has_value());
    CHECK(queen->color() == Color::kBlack);
    CHECK(queen->kind() == PieceKind::kQueen);

    CHECK_FALSE(parsed.board.pieceAt(Position{1, 1}).has_value());
}

TEST_CASE("collects the command lines after the board") {
    std::istringstream in(
        "Board:\n"
        "wK\n"
        "Commands:\n"
        "click 50 50\n"
        "wait 1000\n"
        "print board\n");

    ParsedInput parsed = parseInput(in);

    REQUIRE(parsed.commands.size() == 3);
    CHECK(parsed.commands[0] == "click 50 50");
    CHECK(parsed.commands[1] == "wait 1000");
    CHECK(parsed.commands[2] == "print board");
}

TEST_CASE("rejects a board whose rows differ in width") {
    std::istringstream in(
        "Board:\n"
        "wK .\n"
        ". . .\n"
        "Commands:\n");

    CHECK_THROWS_AS(parseInput(in), ParseError);
}

TEST_CASE("rejects an unknown cell token") {
    std::istringstream in(
        "Board:\n"
        "xZ\n"
        "Commands:\n");

    CHECK_THROWS_AS(parseInput(in), ParseError);
}
