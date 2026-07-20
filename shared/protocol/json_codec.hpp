#pragma once

#include <optional>
#include <string>

#include "protocol/messages.hpp"

namespace kfc::protocol {

// Serialise a message to a compact JSON string for the wire.
std::string encode(const Message& message);

// Parse a JSON string back into a message. Returns nullopt when the text is not
// valid JSON, names no known message type, or is missing a required field.
std::optional<Message> decode(const std::string& text);

}
