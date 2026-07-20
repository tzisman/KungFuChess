#include "protocol/json_codec.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include "io/piece_codec.hpp"
#include "protocol/position_codec.hpp"

namespace kfc::protocol {
namespace {

using nlohmann::json;

// Field names are defined once so encode and decode cannot disagree.
constexpr char kTypeKey[] = "type";
constexpr char kNameKey[] = "name";
constexpr char kColorKey[] = "color";
constexpr char kReasonKey[] = "reason";
constexpr char kFromKey[] = "from";
constexpr char kToKey[] = "to";
constexpr char kCellKey[] = "cell";

constexpr char kJoinType[] = "join";
constexpr char kAssignedType[] = "assigned";
constexpr char kRejectedType[] = "rejected";
constexpr char kMoveType[] = "move";
constexpr char kJumpType[] = "jump";

std::string encodeColor(model::Color color) {
    return std::string(1, io::colorLetter(color));
}

std::optional<model::Color> decodeColor(const json& value) {
    if (!value.is_string()) return std::nullopt;
    const std::string letter = value.get<std::string>();
    if (letter.size() != 1) return std::nullopt;
    return io::colorFromLetter(letter[0]);
}

struct EncodeVisitor {
    json operator()(const JoinRequest& message) const {
        return {{kTypeKey, kJoinType}, {kNameKey, message.name}};
    }
    json operator()(const Assigned& message) const {
        return {{kTypeKey, kAssignedType}, {kColorKey, encodeColor(message.color)}};
    }
    json operator()(const Rejected& message) const {
        return {{kTypeKey, kRejectedType}, {kReasonKey, message.reason}};
    }
    json operator()(const MoveIntent& message) const {
        return {{kTypeKey, kMoveType},
                {kFromKey, encodePosition(message.from)},
                {kToKey, encodePosition(message.to)}};
    }
    json operator()(const JumpIntent& message) const {
        return {{kTypeKey, kJumpType}, {kCellKey, encodePosition(message.cell)}};
    }
};

std::optional<std::string> stringField(const json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_string()) return std::nullopt;
    return object[key].get<std::string>();
}

}  // namespace

std::string encode(const Message& message) {
    return std::visit(EncodeVisitor{}, message).dump();
}

std::optional<Message> decode(const std::string& text) {
    json parsed = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (!parsed.is_object()) return std::nullopt;

    std::optional<std::string> type = stringField(parsed, kTypeKey);
    if (!type) return std::nullopt;

    if (*type == kJoinType) {
        if (std::optional<std::string> name = stringField(parsed, kNameKey))
            return Message{JoinRequest{*name}};
        return std::nullopt;
    }
    if (*type == kAssignedType) {
        if (!parsed.contains(kColorKey)) return std::nullopt;
        if (std::optional<model::Color> color = decodeColor(parsed[kColorKey]))
            return Message{Assigned{*color}};
        return std::nullopt;
    }
    if (*type == kRejectedType) {
        if (std::optional<std::string> reason = stringField(parsed, kReasonKey))
            return Message{Rejected{*reason}};
        return std::nullopt;
    }
    if (*type == kMoveType) {
        if (!parsed.contains(kFromKey) || !parsed.contains(kToKey))
            return std::nullopt;
        std::optional<model::Position> from = decodePosition(parsed[kFromKey]);
        std::optional<model::Position> to = decodePosition(parsed[kToKey]);
        if (!from || !to) return std::nullopt;
        return Message{MoveIntent{*from, *to}};
    }
    if (*type == kJumpType) {
        if (!parsed.contains(kCellKey)) return std::nullopt;
        std::optional<model::Position> cell = decodePosition(parsed[kCellKey]);
        if (!cell) return std::nullopt;
        return Message{JumpIntent{*cell}};
    }
    return std::nullopt;
}

}
