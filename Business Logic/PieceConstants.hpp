#pragma once

namespace kfc::logic {

inline constexpr char kWhiteColor = 'w';
inline constexpr char kBlackColor = 'b';
inline constexpr const char* kEmptyCellToken = ".";


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
