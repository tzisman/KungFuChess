#include "model/piece_cost.hpp"

#include <array>

namespace kfc::model {
namespace {

// The king carries no cost: taking it ends the game outright, so what it is
// worth is the win itself rather than material.
constexpr int kKingCost = 0;
constexpr int kQueenCost = 9;
constexpr int kRookCost = 5;
constexpr int kBishopCost = 3;
constexpr int kKnightCost = 3;
constexpr int kPawnCost = 1;
constexpr int kUnknownCost = 0;

struct KindCost {
    PieceKind kind;
    int cost;
};

constexpr std::array<KindCost, kAllPieceKinds.size()> kKindCosts{{
    {PieceKind::kKing, kKingCost},
    {PieceKind::kQueen, kQueenCost},
    {PieceKind::kRook, kRookCost},
    {PieceKind::kBishop, kBishopCost},
    {PieceKind::kKnight, kKnightCost},
    {PieceKind::kPawn, kPawnCost},
}};

}  // namespace

int costOf(PieceKind kind) {
    for (const KindCost& entry : kKindCosts) {
        if (entry.kind == kind) return entry.cost;
    }
    return kUnknownCost;
}

}
