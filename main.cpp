#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "engine/game_engine.hpp"
#include "input/board_mapper.hpp"
#include "input/controller.hpp"
#include "io/board_parser.hpp"
#include "texttests/script_parser.hpp"
#include "texttests/script_runner.hpp"

//https://github.com/tzisman/KungFuChess

int main() {
    kfc::io::ParsedInput parsed = kfc::io::parseInput(std::cin);

    kfc::engine::GameEngine engine{std::move(parsed.board)};
    kfc::engine::GameSnapshot snapshot = engine.snapshot();
    kfc::input::BoardMapper mapper{snapshot.width(), snapshot.height()};
    kfc::input::Controller controller{engine, mapper};
    kfc::texttests::ScriptRunner runner{controller, engine, std::cout};

    for (const std::string& line : parsed.commands) {
        if (std::optional<kfc::texttests::Command> command =
                kfc::texttests::parseCommand(line)) {
            runner.run(*command);
        }
    }

    return 0;
}
