#include "PieceRegistry.hpp"

#include "Pieces.hpp"

namespace kfc::logic {

PieceRegistry::PieceRegistry(std::initializer_list<const Piece*> pieces) {
    for (const Piece* piece : pieces) {
        bySymbol_.emplace(piece->symbol(), piece);
    }
}

const Piece* PieceRegistry::pieceFor(char symbol) const {
    auto it = bySymbol_.find(symbol);
    return it == bySymbol_.end() ? nullptr : it->second;
}

bool PieceRegistry::isValidSymbol(char symbol) const {
    return bySymbol_.find(symbol) != bySymbol_.end();
}

const PieceRegistry& pieceRegistry() {
    static const King king;
    static const Queen queen;
    static const Rook rook;
    static const Bishop bishop;
    static const Knight knight;
    static const Pawn pawn;
    static const PieceRegistry registry{&king, &queen, &rook, &bishop, &knight, &pawn};
    return registry;
}

}
