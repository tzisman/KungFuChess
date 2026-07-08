#include <iostream>
#include <vector>

#include "Business Logic/BoardParser.h"
#include "Business Logic/Game.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {

kfc::logic::Position pixelToCell(int x, int y) {
    return kfc::logic::Position{y / 100, x / 100};
}

}

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

    for (const auto& command : commands) {
        std::vector<std::string> tokens = kfc::logic::tokenize(command);
        if (tokens.empty()) continue;

        if (tokens[0] == "click" && tokens.size() == 3) {
            int x = std::stoi(tokens[1]);
            int y = std::stoi(tokens[2]);
            game.handleClickCell(pixelToCell(x, y));
        } else if (tokens[0] == "wait" && tokens.size() == 2) {
            game.advanceClock(std::stoll(tokens[1]));
        } else if (command == "print board") {
            kfc::logic::printBoard(game.board(), std::cout);
        }
    }

    return 0;
}
