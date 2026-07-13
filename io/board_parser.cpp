#include "io/board_parser.hpp"

#include <optional>
#include <utility>

#include "io/piece_codec.hpp"
#include "io/text.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::io {
namespace {

constexpr char kBoardMarker[] = "Board:";
constexpr char kCommandsMarker[] = "Commands:";
constexpr char kRowWidthMismatch[] = "ROW_WIDTH_MISMATCH";
constexpr char kUnknownToken[] = "UNKNOWN_TOKEN";

void skipToBoardSection(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        if (trim(line) == kBoardMarker) return;
    }
}

std::vector<std::string> readBoardRows(std::istream& in) {
    std::vector<std::string> rows;
    std::string line;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (trimmed == kCommandsMarker) break;
        if (!trimmed.empty()) rows.push_back(trimmed);
    }
    return rows;
}

std::vector<std::string> readCommandLines(std::istream& in) {
    std::vector<std::string> commands;
    std::string line;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) commands.push_back(trimmed);
    }
    return commands;
}

model::Board buildBoard(const std::vector<std::string>& rows) {
    std::vector<std::vector<std::string>> grid;
    grid.reserve(rows.size());
    for (const std::string& row : rows) grid.push_back(tokenize(row));

    int height = static_cast<int>(grid.size());
    int width = grid.empty() ? 0 : static_cast<int>(grid.front().size());
    for (const std::vector<std::string>& cells : grid) {
        if (static_cast<int>(cells.size()) != width) {
            throw ParseError{kRowWidthMismatch};
        }
    }

    model::Board board{width, height};
    model::PieceId nextId = 1;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const std::string& token = grid[row][col];
            if (isEmptyToken(token)) continue;
            std::optional<PieceCode> code = pieceFromToken(token);
            if (!code) throw ParseError{kUnknownToken};
            board.addPiece(model::Piece{nextId++, code->color, code->kind,
                                        model::Position{row, col}});
        }
    }
    return board;
}

}  

ParsedInput parseInput(std::istream& in) {
    skipToBoardSection(in);
    std::vector<std::string> rows = readBoardRows(in);
    std::vector<std::string> commands = readCommandLines(in);
    return ParsedInput{buildBoard(rows), std::move(commands)};
}

}
