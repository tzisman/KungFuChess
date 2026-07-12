#include "rules/rule_engine.hpp"

#include "model/piece.hpp"
#include "rules/piece_rules.hpp"

namespace kfc::rules {

using model::Board;
using model::Piece;
using model::Position;

namespace {

bool bothInBounds(const Board& board, Position from, Position to) {
    return board.inBounds(from) && board.inBounds(to);
}

bool isFriendlyDestination(const Board& board, Position to, const Piece& mover) {
    auto occupant = board.pieceAt(to);
    return occupant.has_value() && occupant->color() == mover.color();
}

bool ruleAllows(const Board& board, const Piece& mover, Position to) {
    return ruleFor(mover.kind()).legalDestinations(board, mover).count(to) != 0;
}

MoveValidation invalid(Reason reason) {
    return MoveValidation{false, reason};
}

MoveValidation valid() {
    return MoveValidation{true, Reason::kOk};
}

}

std::string reasonCode(Reason reason) {
    switch (reason) {
        case Reason::kOk:                  return "ok";
        case Reason::kOutsideBoard:        return "outside_board";
        case Reason::kEmptySource:         return "empty_source";
        case Reason::kFriendlyDestination: return "friendly_destination";
        case Reason::kIllegalPieceMove:    return "illegal_piece_move";
    }
    return "ok";
}

MoveValidation RuleEngine::validate(const Board& board, Position from,
                                    Position to) const {
    if (!bothInBounds(board, from, to)) {
        return invalid(Reason::kOutsideBoard);
    }

    auto mover = board.pieceAt(from);
    if (!mover.has_value()) {
        return invalid(Reason::kEmptySource);
    }

    if (isFriendlyDestination(board, to, *mover)) {
        return invalid(Reason::kFriendlyDestination);
    }

    if (!ruleAllows(board, *mover, to)) {
        return invalid(Reason::kIllegalPieceMove);
    }

    return valid();
}

}
