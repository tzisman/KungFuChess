#pragma once

#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace kfc::logic {

using Row = std::vector<std::string>;

struct ParseError {
    std::string code;
};

inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

inline bool isValidToken(const std::string& token) {
    if (token == ".") return true;
    if (token.size() != 2) return false;
    char color = token[0];
    char piece = token[1];
    if (color != 'w' && color != 'b') return false;
    switch (piece) {
        case 'K': case 'Q': case 'R': case 'B': case 'N': case 'P':
            return true;
        default:
            return false;
    }
}


inline std::vector<Row> parseBoard(std::istream& in, std::vector<std::string>& commands) {
    std::string line;

    while (std::getline(in, line)) {
        if (trim(line) == "Board:") break;
    }

    std::vector<Row> rows;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (trimmed == "Commands:") break;
        if (trimmed.empty()) continue;

        Row row = tokenize(trimmed);
        for (const auto& token : row) {
            if (!isValidToken(token)) {
                throw ParseError{"UNKNOWN_TOKEN"};
            }
        }
        rows.push_back(row);
    }

    if (!rows.empty()) {
        size_t width = rows.front().size();
        for (const auto& row : rows) {
            if (row.size() != width) {
                throw ParseError{"ROW_WIDTH_MISMATCH"};
            }
        }
    }

    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) commands.push_back(trimmed);
    }

    return rows;
}

inline void printBoard(const std::vector<Row>& rows, std::ostream& out) {
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) out << ' ';
            out << row[i];
        }
        out << '\n';
    }
}

} 
