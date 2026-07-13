#include "io/piece_codec.hpp"

#include <array>

namespace kfc::io {
namespace {

struct KindLetter {
    model::PieceKind kind;
    char letter;
};

constexpr std::array<KindLetter, 6> kKindLetters{{
    {model::PieceKind::kKing, 'K'},
    {model::PieceKind::kQueen, 'Q'},
    {model::PieceKind::kRook, 'R'},
    {model::PieceKind::kBishop, 'B'},
    {model::PieceKind::kKnight, 'N'},
    {model::PieceKind::kPawn, 'P'},
}};

char letterOf(model::PieceKind kind) {
    for (const KindLetter& entry : kKindLetters)
        if (entry.kind == kind) return entry.letter;
    return '?';
}

std::optional<model::PieceKind> kindOf(char letter) {
    for (const KindLetter& entry : kKindLetters)
        if (entry.letter == letter) return entry.kind;
    return std::nullopt;
}

std::optional<model::Color> colorOf(char c) {
    if (c == kWhiteChar) return model::Color::kWhite;
    if (c == kBlackChar) return model::Color::kBlack;
    return std::nullopt;
}

char charOf(model::Color color) {
    return color == model::Color::kWhite ? kWhiteChar : kBlackChar;
}

}  // namespace

bool isEmptyToken(const std::string& token) {
    return token.size() == 1 && token[0] == kEmptyToken;
}

std::optional<PieceCode> pieceFromToken(const std::string& token) {
    if (token.size() != 2) return std::nullopt;
    std::optional<model::Color> color = colorOf(token[0]);
    std::optional<model::PieceKind> kind = kindOf(token[1]);
    if (!color || !kind) return std::nullopt;
    return PieceCode{*color, *kind};
}

std::string encodeCell(const std::optional<model::Piece>& cell) {
    if (!cell) return std::string(1, kEmptyToken);
    std::string token;
    token += charOf(cell->color());
    token += letterOf(cell->kind());
    return token;
}

}
