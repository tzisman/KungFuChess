#pragma once

#include <optional>

#include <nlohmann/json.hpp>

#include "model/position.hpp"

namespace kfc::protocol {

// The one JSON shape a Position takes on the wire: {"row":R,"col":C}. Every
// message that carries a cell shares this, so the shape cannot drift between
// wire_snapshot and the intent messages in json_codec.
nlohmann::json encodePosition(model::Position cell);
std::optional<model::Position> decodePosition(const nlohmann::json& value);

}
