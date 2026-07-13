#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include "texttests/script_parser.hpp"

using kfc::texttests::ClickCommand;
using kfc::texttests::Command;
using kfc::texttests::JumpCommand;
using kfc::texttests::parseCommand;
using kfc::texttests::PrintBoardCommand;
using kfc::texttests::WaitCommand;

TEST_CASE("parses a click command into raw pixel coordinates") {
    std::optional<Command> command = parseCommand("click 50 150");

    REQUIRE(command.has_value());
    const ClickCommand* click = std::get_if<ClickCommand>(&*command);
    REQUIRE(click != nullptr);
    CHECK(click->x == 50);
    CHECK(click->y == 150);
}

TEST_CASE("parses a wait command") {
    std::optional<Command> command = parseCommand("wait 1000");

    REQUIRE(command.has_value());
    const WaitCommand* wait = std::get_if<WaitCommand>(&*command);
    REQUIRE(wait != nullptr);
    CHECK(wait->ms == 1000);
}

TEST_CASE("parses a print board command") {
    std::optional<Command> command = parseCommand("print board");

    REQUIRE(command.has_value());
    CHECK(std::holds_alternative<PrintBoardCommand>(*command));
}

TEST_CASE("parses a jump command into raw pixel coordinates") {
    std::optional<Command> command = parseCommand("jump 50 150");

    REQUIRE(command.has_value());
    const JumpCommand* jump = std::get_if<JumpCommand>(&*command);
    REQUIRE(jump != nullptr);
    CHECK(jump->x == 50);
    CHECK(jump->y == 150);
}

TEST_CASE("returns nothing for an unknown or malformed line") {
    CHECK_FALSE(parseCommand("").has_value());
    CHECK_FALSE(parseCommand("jump 1").has_value());
    CHECK_FALSE(parseCommand("wait").has_value());
    CHECK_FALSE(parseCommand("click 1 two").has_value());
}
