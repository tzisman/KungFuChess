#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "app/controller_factory.hpp"
#include "engine/game_engine.hpp"
#include "input/controller.hpp"
#include "input/engine_command_sink.hpp"
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
        kfc::input::EngineCommandSink commands{engine};
        kfc::input::Controller controller = kfc::app::makeController(
            board, commands,
            kfc::view::BoardGeometry{
                board.width() * kfc::texttests::kCellSize,
                board.height() * kfc::texttests::kCellSize, board.width(),
                board.height()});
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
