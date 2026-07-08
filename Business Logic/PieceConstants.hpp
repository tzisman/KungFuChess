#pragma once

namespace kfc::logic {

inline constexpr char kWhiteColor = 'w';
inline constexpr char kBlackColor = 'b';
inline constexpr const char* kEmptyCellToken = ".";

// Single source of truth for every piece's board letter. Each concrete piece
// reports its own symbol from here, and any rule that names another piece
// (e.g. pawn promotion targeting the queen) references these constants rather
// than an inline literal.
namespace symbols {
inline constexpr char King = 'K';
inline constexpr char Queen = 'Q';
inline constexpr char Rook = 'R';
inline constexpr char Bishop = 'B';
inline constexpr char Knight = 'N';
inline constexpr char Pawn = 'P';
}

inline bool isValidColor(char c) { return c == kWhiteColor || c == kBlackColor; }

}
