#pragma once

#include <optional>

namespace kfc::logic {

enum class PieceType { King, Queen, Rook, Bishop, Knight, Pawn };

constexpr char kWhiteColor = 'w';
constexpr char kBlackColor = 'b';
constexpr const char* kEmptyCellToken = ".";

inline bool isValidColor(char c) { return c == kWhiteColor || c == kBlackColor; }

inline std::optional<PieceType> charToPieceType(char c) {
    switch (c) {
        case 'K': return PieceType::King;
        case 'Q': return PieceType::Queen;
        case 'R': return PieceType::Rook;
        case 'B': return PieceType::Bishop;
        case 'N': return PieceType::Knight;
        case 'P': return PieceType::Pawn;
        default: return std::nullopt;
    }
}

}
