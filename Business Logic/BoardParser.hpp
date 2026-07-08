#pragma once

#include <istream>
#include <string>
#include <vector>

#include "Board.hpp"

namespace kfc::logic {

struct ParseError {
    std::string code;
};

std::string trim(const std::string& s);
std::vector<std::string> tokenize(const std::string& line);
bool isValidToken(const std::string& token);

void skipToBoardMarker(std::istream& in);
std::vector<Row> readBoardRows(std::istream& in);
void validateBoard(const std::vector<Row>& rows);
std::vector<std::string> readCommands(std::istream& in);

std::vector<Row> parseBoard(std::istream& in, std::vector<std::string>& commands);

}
