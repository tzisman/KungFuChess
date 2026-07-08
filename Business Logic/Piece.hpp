#pragma once

#include <optional>

#include "Board.hpp"

namespace kfc::logic {

// A piece encapsulates all per-type rules. Instances are stateless singletons
// (one shared object per type, owned by the registry): everything that varies
// between board occurrences — colour, position — is passed in as a parameter,
// never stored. Shared, type-agnostic logic (path scanning, travel time) lives
// in MoveRules and is not the piece's concern; a piece only declares *whether*
// it needs a clear path, not *how* clearance is checked.
class Piece {
public:
    virtual ~Piece() = default;

    // The board letter identifying this type.
    virtual char symbol() const = 0;

    // Rule 1 — is the move's geometry legal for this type? Pure geometry
    // (plus colour for direction-dependent pieces such as the pawn).
    virtual bool isShapeLegal(char color, Position from, Position to,
                              int boardHeight) const = 0;

    // Rule 2 — must every square between from and to be empty?
    virtual bool requiresClearPath() const = 0;

    // Rule 3 — a constraint on the destination's contents beyond shape and
    // path. Default: none. The pawn overrides this (diagonal must capture,
    // straight must be empty).
    virtual bool isDestinationLegal(const Board& board, char color,
                                    Position from, Position to) const;

    // Does capturing this piece end the game? Default: no. The king overrides.
    virtual bool isRoyal() const;

    // After arriving at `at`, does this piece transform? Returns the new
    // symbol, or nullopt for no transformation. The pawn overrides this to
    // promote on reaching its far row.
    virtual std::optional<char> promotionSymbol(char color, Position at,
                                                int boardHeight) const;
};

}
