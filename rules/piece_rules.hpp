#pragma once

#include <optional>
#include <set>

#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::rules {

using Destinations = std::set<model::Position>;


class PieceRule {
public:
    virtual ~PieceRule() = default;
    virtual Destinations legalDestinations(const model::Board& board,
                                           const model::Piece& piece) const = 0;
};

class RookRule : public PieceRule {
public:
    Destinations legalDestinations(const model::Board& board,
                                   const model::Piece& piece) const override;
};

class BishopRule : public PieceRule {
public:
    Destinations legalDestinations(const model::Board& board,
                                   const model::Piece& piece) const override;
};

class QueenRule : public PieceRule {
public:
    Destinations legalDestinations(const model::Board& board,
                                   const model::Piece& piece) const override;
};

class KnightRule : public PieceRule {
public:
    Destinations legalDestinations(const model::Board& board,
                                   const model::Piece& piece) const override;
};

class KingRule : public PieceRule {
public:
    Destinations legalDestinations(const model::Board& board,
                                   const model::Piece& piece) const override;
};

class PawnRule : public PieceRule {
public:
    Destinations legalDestinations(const model::Board& board,
                                   const model::Piece& piece) const override;
};

const PieceRule& ruleFor(model::PieceKind kind);

std::optional<model::PieceKind> promotedKind(const model::Board& board,
                                             const model::Piece& piece);

}
