#pragma once

#include <string>

#include "model/board.hpp"
#include "model/position.hpp"
#include "rules/piece_rules.hpp"

namespace kfc::rules {

enum class Reason {
    kOk,
    kOutsideBoard,
    kEmptySource,
    kFriendlyDestination,
    kIllegalPieceMove,
};

struct MoveValidation {
    bool isValid;
    Reason reason;
};

std::string reasonCode(Reason reason);

class RuleEngine {
public:
    MoveValidation validate(const model::Board& board,
                            model::Position from,
                            model::Position to) const;

    Destinations legalDestinations(const model::Board& board,
                                   model::Position from) const;
};

}
