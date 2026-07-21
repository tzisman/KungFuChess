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
constexpr char kRatingKey[] = "rating";
constexpr char kOpponentNameKey[] = "opponent_name";
constexpr char kFromKey[] = "from";
constexpr char kToKey[] = "to";
constexpr char kCellKey[] = "cell";
constexpr char kSecondsLeftKey[] = "seconds_left";
constexpr char kRoomNameKey[] = "room_name";
constexpr char kRoleKey[] = "role";

constexpr char kRegisterType[] = "register";
constexpr char kLoginType[] = "login";
constexpr char kRegisteredType[] = "registered";
constexpr char kAuthRejectedType[] = "auth_rejected";
constexpr char kLoggedInType[] = "logged_in";
constexpr char kPlayRequestType[] = "play_request";
constexpr char kMatchedType[] = "matched";
constexpr char kNoOpponentType[] = "no_opponent";
constexpr char kMoveType[] = "move";
constexpr char kJumpType[] = "jump";
constexpr char kCountdownTickType[] = "countdown_tick";
constexpr char kEnterRoomType[] = "enter_room";
constexpr char kRoomJoinedType[] = "room_joined";

constexpr char kWhiteRole[] = "white";
constexpr char kBlackRole[] = "black";
constexpr char kSpectatorRole[] = "spectator";

std::string encodeColor(model::Color color) {
    return std::string(1, io::colorLetter(color));
}

std::optional<model::Color> decodeColor(const json& value) {
    if (!value.is_string()) return std::nullopt;
    const std::string letter = value.get<std::string>();
    if (letter.size() != 1) return std::nullopt;
    return io::colorFromLetter(letter[0]);
}

std::string encodeRole(Role role) {
    switch (role) {
        case Role::kWhite: return kWhiteRole;
        case Role::kBlack: return kBlackRole;
        case Role::kSpectator: return kSpectatorRole;
    }
    return kSpectatorRole;
}

std::optional<Role> decodeRole(const json& value) {
    if (!value.is_string()) return std::nullopt;
    const std::string name = value.get<std::string>();
    if (name == kWhiteRole) return Role::kWhite;
    if (name == kBlackRole) return Role::kBlack;
    if (name == kSpectatorRole) return Role::kSpectator;
    return std::nullopt;
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
    json operator()(const LoggedIn& message) const {
        return {{kTypeKey, kLoggedInType}, {kRatingKey, message.rating}};
    }
    json operator()(const PlayRequest&) const {
        return {{kTypeKey, kPlayRequestType}};
    }
    json operator()(const Matched& message) const {
        return {{kTypeKey, kMatchedType},
                {kColorKey, encodeColor(message.color)},
                {kOpponentNameKey, message.opponentName}};
    }
    json operator()(const NoOpponent&) const {
        return {{kTypeKey, kNoOpponentType}};
    }
    json operator()(const MoveIntent& message) const {
        return {{kTypeKey, kMoveType},
                {kFromKey, encodePosition(message.from)},
                {kToKey, encodePosition(message.to)}};
    }
    json operator()(const JumpIntent& message) const {
        return {{kTypeKey, kJumpType}, {kCellKey, encodePosition(message.cell)}};
    }
    json operator()(const CountdownTick& message) const {
        return {{kTypeKey, kCountdownTickType}, {kSecondsLeftKey, message.secondsLeft}};
    }
    json operator()(const EnterRoomRequest& message) const {
        return {{kTypeKey, kEnterRoomType}, {kRoomNameKey, message.roomName}};
    }
    json operator()(const RoomJoined& message) const {
        json result{{kTypeKey, kRoomJoinedType}, {kRoleKey, encodeRole(message.role)}};
        if (message.color) result[kColorKey] = encodeColor(*message.color);
        return result;
    }
};

std::optional<std::string> stringField(const json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_string()) return std::nullopt;
    return object[key].get<std::string>();
}

std::optional<int> intField(const json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_number_integer()) return std::nullopt;
    return object[key].get<int>();
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
    if (*type == kLoggedInType) {
        if (std::optional<int> rating = intField(parsed, kRatingKey))
            return Message{LoggedIn{*rating}};
        return std::nullopt;
    }
    if (*type == kPlayRequestType) {
        return Message{PlayRequest{}};
    }
    if (*type == kMatchedType) {
        if (!parsed.contains(kColorKey)) return std::nullopt;
        std::optional<model::Color> color = decodeColor(parsed[kColorKey]);
        std::optional<std::string> opponentName = stringField(parsed, kOpponentNameKey);
        if (!color || !opponentName) return std::nullopt;
        return Message{Matched{*color, *opponentName}};
    }
    if (*type == kNoOpponentType) {
        return Message{NoOpponent{}};
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
    if (*type == kCountdownTickType) {
        if (std::optional<int> secondsLeft = intField(parsed, kSecondsLeftKey))
            return Message{CountdownTick{*secondsLeft}};
        return std::nullopt;
    }
    if (*type == kEnterRoomType) {
        if (std::optional<std::string> roomName = stringField(parsed, kRoomNameKey))
            return Message{EnterRoomRequest{*roomName}};
        return std::nullopt;
    }
    if (*type == kRoomJoinedType) {
        if (!parsed.contains(kRoleKey)) return std::nullopt;
        std::optional<Role> role = decodeRole(parsed[kRoleKey]);
        if (!role) return std::nullopt;
        std::optional<model::Color> color;
        if (parsed.contains(kColorKey)) {
            color = decodeColor(parsed[kColorKey]);
            if (!color) return std::nullopt;
        }
        return Message{RoomJoined{*role, color}};
    }
    return std::nullopt;
}

}
