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
constexpr char kUsernameKey[] = "username";
constexpr char kPasswordKey[] = "password";
constexpr char kColorKey[] = "color";
constexpr char kReasonKey[] = "reason";
constexpr char kFromKey[] = "from";
constexpr char kToKey[] = "to";
constexpr char kCellKey[] = "cell";

constexpr char kRegisterType[] = "register";
constexpr char kLoginType[] = "login";
constexpr char kRegisteredType[] = "registered";
constexpr char kAuthRejectedType[] = "auth_rejected";
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
    json operator()(const RegisterRequest& message) const {
        return {{kTypeKey, kRegisterType},
                {kUsernameKey, message.username},
                {kPasswordKey, message.password}};
    }
    json operator()(const LoginRequest& message) const {
        return {{kTypeKey, kLoginType},
                {kUsernameKey, message.username},
                {kPasswordKey, message.password}};
    }
    json operator()(const Registered&) const {
        return {{kTypeKey, kRegisteredType}};
    }
    json operator()(const AuthRejected& message) const {
        return {{kTypeKey, kAuthRejectedType}, {kReasonKey, message.reason}};
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
    // A user-supplied field (a player's name) can carry bytes that are not
    // valid UTF-8 if it came from a misconfigured console or a misbehaving
    // client. Replacing rather than throwing keeps one bad name from crashing
    // whichever side — client or server — encodes it.
    return std::visit(EncodeVisitor{}, message)
        .dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

std::optional<Message> decode(const std::string& text) {
    json parsed = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (!parsed.is_object()) return std::nullopt;

    std::optional<std::string> type = stringField(parsed, kTypeKey);
    if (!type) return std::nullopt;

    if (*type == kRegisterType) {
        std::optional<std::string> username = stringField(parsed, kUsernameKey);
        std::optional<std::string> password = stringField(parsed, kPasswordKey);
        if (!username || !password) return std::nullopt;
        return Message{RegisterRequest{*username, *password}};
    }
    if (*type == kLoginType) {
        std::optional<std::string> username = stringField(parsed, kUsernameKey);
        std::optional<std::string> password = stringField(parsed, kPasswordKey);
        if (!username || !password) return std::nullopt;
        return Message{LoginRequest{*username, *password}};
    }
    if (*type == kRegisteredType) {
        return Message{Registered{}};
    }
    if (*type == kAuthRejectedType) {
        if (std::optional<std::string> reason = stringField(parsed, kReasonKey))
            return Message{AuthRejected{*reason}};
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
