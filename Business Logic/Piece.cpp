#include "Piece.hpp"

namespace kfc::logic {

bool Piece::isDestinationLegal(const Board&, char, Position, Position) const {
    return true;
}

bool Piece::isRoyal() const { return false; }

std::optional<char> Piece::promotionSymbol(char, Position, int) const {
    return std::nullopt;
}

}
