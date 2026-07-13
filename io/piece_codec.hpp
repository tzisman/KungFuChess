#pragma once

#include <optional>
#include <string>

#include "model/piece.hpp"

namespace kfc::io {

inline constexpr char kEmptyToken = '.';
inline constexpr char kWhiteChar = 'w';
inline constexpr char kBlackChar = 'b';

struct PieceCode {
    model::Color color;
    model::PieceKind kind;
};

bool isEmptyToken(const std::string& token);
std::optional<PieceCode> pieceFromToken(const std::string& token);
std::string encodeCell(const std::optional<model::Piece>& cell);

}
