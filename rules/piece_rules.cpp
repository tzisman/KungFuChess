#include "rules/piece_rules.hpp"

#include <vector>

namespace kfc::rules {

using model::Board;
using model::Color;
using model::Piece;
using model::PieceKind;
using model::Position;

namespace {

struct Offset {
    int dRow;
    int dCol;
};

const std::vector<Offset> kRookDirections = {
    {+1, 0}, {-1, 0}, {0, +1}, {0, -1},
};

const std::vector<Offset> kBishopDirections = {
    {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1},
};

const std::vector<Offset> kKnightOffsets = {
    {+2, +1}, {+2, -1}, {-2, +1}, {-2, -1},
    {+1, +2}, {+1, -2}, {-1, +2}, {-1, -2},
};

const std::vector<Offset> kKingOffsets = {
    {+1, 0}, {-1, 0}, {0, +1}, {0, -1},
    {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1},
};

const int kWhiteForward = -1;
const int kBlackForward = +1;
const int kBlackStartRow = 1;
const int kWhiteStartOffset = 2;
const int kPawnDoubleStep = 2;
const PieceKind kPromotionKind = PieceKind::kQueen;

Position shifted(Position cell, const Offset& offset) {
    return Position{cell.row + offset.dRow, cell.col + offset.dCol};
}

bool isEmpty(const Board& board, Position cell) {
    return !board.pieceAt(cell).has_value();
}

bool isEnemy(const Board& board, Position cell, const Piece& mover) {
    auto occupant = board.pieceAt(cell);
    return occupant.has_value() && occupant->color() != mover.color();
}


Destinations slide(const Board& board, const Piece& piece,
                   const std::vector<Offset>& directions) {
    Destinations destinations;
    for (const Offset& direction : directions) {
        Position cell = shifted(piece.cell(), direction);
        while (board.inBounds(cell)) {
            if (isEmpty(board, cell)) {
                destinations.insert(cell);
                cell = shifted(cell, direction);
                continue;
            }
            if (isEnemy(board, cell, piece)) {
                destinations.insert(cell);
            }
            break;
        }
    }
    return destinations;
}


Destinations steps(const Board& board, const Piece& piece,
                   const std::vector<Offset>& offsets) {
    Destinations destinations;
    for (const Offset& offset : offsets) {
        Position cell = shifted(piece.cell(), offset);
        if (board.inBounds(cell) &&
            (isEmpty(board, cell) || isEnemy(board, cell, piece))) {
            destinations.insert(cell);
        }
    }
    return destinations;
}

int forwardOf(Color color) {
    return color == Color::kWhite ? kWhiteForward : kBlackForward;
}

int startRowOf(Color color, int boardHeight) {
    return color == Color::kWhite ? boardHeight - kWhiteStartOffset
                                  : kBlackStartRow;
}

int promotionRow(Color color, int boardHeight) {
    return forwardOf(color) > 0 ? boardHeight - 1 : 0;
}

void addPawnAdvance(const Board& board, const Piece& piece,
                    Destinations& destinations) {
    const int forward = forwardOf(piece.color());
    const Position one = shifted(piece.cell(), Offset{forward, 0});
    if (!board.inBounds(one) || !isEmpty(board, one)) {
        return;
    }
    destinations.insert(one);

    if (piece.cell().row != startRowOf(piece.color(), board.height())) {
        return;
    }
    const Position two =
        shifted(piece.cell(), Offset{forward * kPawnDoubleStep, 0});
    if (board.inBounds(two) && isEmpty(board, two)) {
        destinations.insert(two);
    }
}

void addPawnCaptures(const Board& board, const Piece& piece,
                     Destinations& destinations) {
    const int forward = forwardOf(piece.color());
    for (int side : {+1, -1}) {
        const Position diagonal = shifted(piece.cell(), Offset{forward, side});
        if (board.inBounds(diagonal) && isEnemy(board, diagonal, piece)) {
            destinations.insert(diagonal);
        }
    }
}

}

Destinations RookRule::legalDestinations(const Board& board,
                                         const Piece& piece) const {
    return slide(board, piece, kRookDirections);
}

Destinations BishopRule::legalDestinations(const Board& board,
                                           const Piece& piece) const {
    return slide(board, piece, kBishopDirections);
}

Destinations QueenRule::legalDestinations(const Board& board,
                                          const Piece& piece) const {
    Destinations destinations = slide(board, piece, kRookDirections);
    const Destinations diagonals = slide(board, piece, kBishopDirections);
    destinations.insert(diagonals.begin(), diagonals.end());
    return destinations;
}

Destinations KnightRule::legalDestinations(const Board& board,
                                           const Piece& piece) const {
    return steps(board, piece, kKnightOffsets);
}

Destinations KingRule::legalDestinations(const Board& board,
                                         const Piece& piece) const {
    return steps(board, piece, kKingOffsets);
}

Destinations PawnRule::legalDestinations(const Board& board,
                                         const Piece& piece) const {
    Destinations destinations;
    addPawnAdvance(board, piece, destinations);
    addPawnCaptures(board, piece, destinations);
    return destinations;
}

const PieceRule& ruleFor(PieceKind kind) {
    static const RookRule kRook;
    static const BishopRule kBishop;
    static const QueenRule kQueen;
    static const KnightRule kKnight;
    static const KingRule kKing;
    static const PawnRule kPawn;
    switch (kind) {
        case PieceKind::kRook:   return kRook;
        case PieceKind::kBishop: return kBishop;
        case PieceKind::kQueen:  return kQueen;
        case PieceKind::kKnight: return kKnight;
        case PieceKind::kKing:   return kKing;
        case PieceKind::kPawn:   return kPawn;
    }
    return kPawn;
}

std::optional<PieceKind> promotedKind(const Board& board, const Piece& piece) {
    if (piece.kind() != PieceKind::kPawn) {
        return std::nullopt;
    }
    if (piece.cell().row == promotionRow(piece.color(), board.height())) {
        return kPromotionKind;
    }
    return std::nullopt;
}

}
