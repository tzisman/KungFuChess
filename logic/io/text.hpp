#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace kfc::io {

inline std::string trim(const std::string& s) {
    static const char* kWhitespace = " \t\r\n";
    size_t start = s.find_first_not_of(kWhitespace);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(kWhitespace);
    return s.substr(start, end - start + 1);
}

inline std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}

}
