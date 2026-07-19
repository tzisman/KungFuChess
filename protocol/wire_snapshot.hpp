#pragma once

#include <optional>
#include <string>

#include "product/game_state_view.hpp"

namespace kfc::protocol {

// The whole game state as it crosses the wire: the neutral read-model from
// product/, serialised so a client with no engine can render it. Encoding the
// GameStateView directly (rather than a parallel struct) keeps one definition
// of what a snapshot is.
std::string encodeSnapshot(const product::GameStateView& state);

// Rebuild the state from its JSON. Returns nullopt when the text is not valid
// JSON or is missing/mistyped in a required field.
std::optional<product::GameStateView> decodeSnapshot(const std::string& text);

}
