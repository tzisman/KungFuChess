#include <iostream>
#include <vector>

#include "App/CommandParser.hpp"
#include "App/CommandRunner.hpp"
#include "Business Logic/BoardParser.hpp"
#include "Business Logic/Game.hpp"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main() {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    std::vector<std::string> commands;
    std::vector<kfc::logic::Row> board;

    try {
        board = kfc::logic::parseBoard(std::cin, commands);
    } catch (const kfc::logic::ParseError& e) {
        std::cout << "ERROR " << e.code << "\n";
        return 0;
    }

    kfc::logic::Game game(std::move(board));
    kfc::app::CommandRunner runner(game, std::cout);

    for (const auto& line : commands) {
        if (auto command = kfc::app::parseCommand(line)) {
            runner.run(*command);
        }
    }

    return 0;
}
