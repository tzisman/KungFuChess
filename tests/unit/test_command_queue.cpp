#include <doctest/doctest.h>

#include <variant>
#include <vector>

#include "model/piece.hpp"
#include "model/position.hpp"
#include "server/command_queue.hpp"

using kfc::model::Color;
using kfc::model::Position;
using kfc::server::CommandQueue;
using kfc::server::JumpCommand;
using kfc::server::MoveCommand;
using kfc::server::PlayerCommand;

TEST_CASE("draining an empty queue yields nothing") {
    CommandQueue queue;

    CHECK(queue.drain().empty());
}

TEST_CASE("drain hands over every command pushed since the last drain, in order") {
    CommandQueue queue;

    queue.push(MoveCommand{Color::kWhite, Position{1, 1}, Position{2, 1}});
    queue.push(JumpCommand{Color::kBlack, Position{6, 4}});

    std::vector<PlayerCommand> drained = queue.drain();
    REQUIRE(drained.size() == 2);
    REQUIRE(std::holds_alternative<MoveCommand>(drained[0]));
    CHECK(std::get<MoveCommand>(drained[0]).color == Color::kWhite);
    REQUIRE(std::holds_alternative<JumpCommand>(drained[1]));
    CHECK(std::get<JumpCommand>(drained[1]).color == Color::kBlack);
}

TEST_CASE("drain leaves nothing behind for the next call") {
    CommandQueue queue;
    queue.push(MoveCommand{Color::kWhite, Position{1, 1}, Position{2, 1}});

    queue.drain();

    CHECK(queue.drain().empty());
}
