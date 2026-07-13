#include "texttests/script_parser.hpp"

#include <exception>
#include <vector>

#include "io/text.hpp"

namespace kfc::texttests {
namespace {

constexpr char kClick[] = "click";
constexpr char kWait[] = "wait";
constexpr char kPrint[] = "print";
constexpr char kBoard[] = "board";

}  // namespace

std::optional<Command> parseCommand(const std::string& line) {
    std::vector<std::string> tokens = io::tokenize(line);
    if (tokens.empty()) return std::nullopt;

    try {
        if (tokens[0] == kClick && tokens.size() == 3)
            return ClickCommand{std::stoi(tokens[1]), std::stoi(tokens[2])};
        if (tokens[0] == kWait && tokens.size() == 2)
            return WaitCommand{std::stoi(tokens[1])};
        if (tokens[0] == kPrint && tokens.size() == 2 && tokens[1] == kBoard)
            return PrintBoardCommand{};
    } catch (const std::exception&) {
        return std::nullopt;
    }

    return std::nullopt;
}

}
