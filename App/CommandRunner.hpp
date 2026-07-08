#pragma once

#include <ostream>

#include "Business Logic/Game.hpp"
#include "CommandParser.hpp"

namespace kfc::app {

// Applies structured commands to a Game. Holds the output stream so only the
// rendering command touches it; the rest stay pure state transitions.
class CommandRunner {
public:
    CommandRunner(logic::Game& game, std::ostream& out);

    void run(const Command& command);

private:
    logic::Game& game_;
    std::ostream& out_;
};

}
