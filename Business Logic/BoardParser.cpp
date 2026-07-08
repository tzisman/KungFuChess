#include "BoardParser.hpp"

#include <sstream>

#include "PieceConstants.hpp"
#include "PieceRegistry.hpp"

namespace kfc::logic {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

bool isValidToken(const std::string& token) {
    if (token == kEmptyCellToken) return true;
    if (token.size() != 2) return false;
    return isValidColor(token[0]) && pieceRegistry().isValidSymbol(token[1]);
}

void skipToBoardMarker(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        if (trim(line) == "Board:") return;
    }
}

std::vector<Row> readBoardRows(std::istream& in) {
    std::vector<Row> rows;
    std::string line;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (trimmed == "Commands:") break;
        if (trimmed.empty()) continue;
        rows.push_back(tokenize(trimmed));
    }
    return rows;
}

void validateBoard(const std::vector<Row>& rows) {
    for (const auto& row : rows) {
        for (const auto& token : row) {
            if (!isValidToken(token)) {
                throw ParseError{"UNKNOWN_TOKEN"};
            }
        }
    }

    if (!rows.empty()) {
        size_t width = rows.front().size();
        for (const auto& row : rows) {
            if (row.size() != width) {
                throw ParseError{"ROW_WIDTH_MISMATCH"};
            }
        }
    }
}

std::vector<std::string> readCommands(std::istream& in) {
    std::vector<std::string> commands;
    std::string line;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) commands.push_back(trimmed);
    }
    return commands;
}

std::vector<Row> parseBoard(std::istream& in, std::vector<std::string>& commands) {
    skipToBoardMarker(in);
    std::vector<Row> rows = readBoardRows(in);
    validateBoard(rows);
    commands = readCommands(in);
    return rows;
}

}
