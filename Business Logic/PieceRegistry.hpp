#pragma once

#include <initializer_list>
#include <unordered_map>

#include "Piece.hpp"

namespace kfc::logic {

// The single place that knows the full set of piece types. Maps a board letter
// to the shared, stateless piece object that owns that type's rules. Adding a
// new piece type means adding one class and one entry to the builder in
// pieceRegistry() — no switch anywhere else has to change.
class PieceRegistry {
public:
    explicit PieceRegistry(std::initializer_list<const Piece*> pieces);

    const Piece* pieceFor(char symbol) const;   // nullptr if unknown
    bool isValidSymbol(char symbol) const;

private:
    std::unordered_map<char, const Piece*> bySymbol_;
};

const PieceRegistry& pieceRegistry();

}
