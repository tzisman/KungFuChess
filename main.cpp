#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "engine/game_engine.hpp"
#include "input/board_mapper.hpp"
#include "input/controller.hpp"
#include "io/board_parser.hpp"
#include "io/board_printer.hpp"
#include "model/board.hpp"
#include "texttests/script_parser.hpp"
#include "texttests/script_runner.hpp"
#include "view/board_geometry.hpp"

//https://github.com/tzisman/KungFuChess

int main() {
    try {
        kfc::io::ParsedInput parsed = kfc::io::parseInput(std::cin);

        kfc::engine::GameEngine engine{std::move(parsed.board)};
        const kfc::model::Board& board = engine.board();
        kfc::input::BoardMapper mapper{kfc::view::BoardGeometry{
            board.width() * kfc::texttests::kCellSize,
            board.height() * kfc::texttests::kCellSize, board.width(),
            board.height()}};
        kfc::input::Controller controller{engine, mapper};
        kfc::texttests::ScriptRunner runner{controller, engine, std::cout};

        for (const std::string& line : parsed.commands) {
            if (std::optional<kfc::texttests::Command> command =
                    kfc::texttests::parseCommand(line)) {
                runner.run(*command);
            }
        }
    } catch (const kfc::io::ParseError& error) {
        kfc::io::printParseError(error, std::cout);
    }

    return 0;
}
