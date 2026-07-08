#include "CommandRunner.hpp"

#include <variant>

#include "Business Logic/BoardRenderer.hpp"

namespace kfc::app {

namespace {
template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}

CommandRunner::CommandRunner(logic::Game& game, std::ostream& out) : game_(game), out_(out) {}

void CommandRunner::run(const Command& command) {
    std::visit(overloaded{
                   [&](const ClickCommand& c) { game_.handleClickCell(c.cell); },
                   [&](const JumpCommand& c) { game_.handleJumpCommand(c.cell); },
                   [&](const WaitCommand& c) { game_.advanceClock(c.ms); },
                   [&](const PrintBoardCommand&) { logic::printBoard(game_.board(), out_); },
               },
               command);
}

}
