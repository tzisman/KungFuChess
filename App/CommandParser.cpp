#include "CommandParser.hpp"

#include <stdexcept>
#include <vector>

#include "Business Logic/BoardParser.hpp"

namespace kfc::app {

logic::Position pixelToCell(int x, int y) {
    return logic::Position{y / kCellSizePixels, x / kCellSizePixels};
}

std::optional<Command> parseCommand(const std::string& line) {
    std::vector<std::string> tokens = logic::tokenize(line);
    if (tokens.empty()) return std::nullopt;

    try {
        if (tokens[0] == "click" && tokens.size() == 3)
            return ClickCommand{pixelToCell(std::stoi(tokens[1]), std::stoi(tokens[2]))};
        if (tokens[0] == "jump" && tokens.size() == 3)
            return JumpCommand{pixelToCell(std::stoi(tokens[1]), std::stoi(tokens[2]))};
        if (tokens[0] == "wait" && tokens.size() == 2)
            return WaitCommand{std::stoll(tokens[1])};
        if (tokens[0] == "print" && tokens.size() == 2 && tokens[1] == "board")
            return PrintBoardCommand{};
    } catch (const std::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

}
