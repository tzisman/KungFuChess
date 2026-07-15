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

// The single definition of which letter denotes a kind or a colour. Shared by
// the text encoding and by any other consumer that names pieces, so the two
// can never drift apart.
char kindLetter(model::PieceKind kind);
char colorLetter(model::Color color);

}
