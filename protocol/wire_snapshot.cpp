#include "protocol/wire_snapshot.hpp"

#include <array>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "engine/game_events.hpp"
#include "io/piece_codec.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "product/move_log.hpp"
#include "protocol/position_codec.hpp"

namespace kfc::protocol {
namespace {

using nlohmann::json;

constexpr char kBoardWidthKey[] = "boardWidth";
constexpr char kBoardHeightKey[] = "boardHeight";
constexpr char kGameOverKey[] = "gameOver";
constexpr char kPiecesKey[] = "pieces";
constexpr char kPlayersKey[] = "players";

constexpr char kKindKey[] = "kind";
constexpr char kColorKey[] = "color";
constexpr char kCellKey[] = "cell";
constexpr char kStateKey[] = "state";
constexpr char kMovingToKey[] = "movingTo";
constexpr char kProgressKey[] = "progress";
constexpr char kElapsedKey[] = "elapsedMs";

constexpr char kNameKey[] = "name";
constexpr char kScoreKey[] = "score";
constexpr char kMovesKey[] = "moves";
constexpr char kActionKey[] = "action";
constexpr char kFromKey[] = "from";
constexpr char kToKey[] = "to";
constexpr char kAtMsKey[] = "atMs";

// The wire codes for the two enums that io does not already letter. Each table
// is the single source shared by encode and decode, so the two cannot drift.
struct StateCode {
    model::PieceState state;
    int code;
};
constexpr std::array<StateCode, 6> kStateCodes{{
    {model::PieceState::kIdle, 0},
    {model::PieceState::kMoving, 1},
    {model::PieceState::kCaptured, 2},
    {model::PieceState::kAirborne, 3},
    {model::PieceState::kShortResting, 4},
    {model::PieceState::kLongResting, 5},
}};

struct ActionCode {
    engine::ActionKind action;
    int code;
};
constexpr std::array<ActionCode, 2> kActionCodes{{
    {engine::ActionKind::kMove, 0},
    {engine::ActionKind::kJump, 1},
}};

int stateCode(model::PieceState state) {
    for (const StateCode& entry : kStateCodes)
        if (entry.state == state) return entry.code;
    return -1;
}
std::optional<model::PieceState> stateFromCode(int code) {
    for (const StateCode& entry : kStateCodes)
        if (entry.code == code) return entry.state;
    return std::nullopt;
}
int actionCode(engine::ActionKind action) {
    for (const ActionCode& entry : kActionCodes)
        if (entry.action == action) return entry.code;
    return -1;
}
std::optional<engine::ActionKind> actionFromCode(int code) {
    for (const ActionCode& entry : kActionCodes)
        if (entry.code == code) return entry.action;
    return std::nullopt;
}

std::string letter(char c) { return std::string(1, c); }

// --- typed field readers: nullopt on missing or wrong-typed field ---

std::optional<int> intField(const json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_number_integer())
        return std::nullopt;
    return object[key].get<int>();
}
std::optional<double> numberField(const json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_number()) return std::nullopt;
    return object[key].get<double>();
}
std::optional<bool> boolField(const json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_boolean()) return std::nullopt;
    return object[key].get<bool>();
}
std::optional<std::string> stringField(const json& object, const char* key) {
    if (!object.contains(key) || !object[key].is_string()) return std::nullopt;
    return object[key].get<std::string>();
}
template <typename T>
std::optional<T> firstCharMapped(const json& object, const char* key,
                                  std::optional<T> (*from)(char)) {
    std::optional<std::string> text = stringField(object, key);
    if (!text || text->size() != 1) return std::nullopt;
    return from((*text)[0]);
}

// --- piece ---

json encodePiece(const product::PieceSnapshot& piece) {
    json out = {{kKindKey, letter(io::kindLetter(piece.kind))},
                {kColorKey, letter(io::colorLetter(piece.color))},
                {kCellKey, encodePosition(piece.cell)},
                {kStateKey, stateCode(piece.state)},
                {kProgressKey, piece.progress},
                {kElapsedKey, piece.stateElapsedMs}};
    if (piece.movingTo) out[kMovingToKey] = encodePosition(*piece.movingTo);
    return out;
}
std::optional<product::PieceSnapshot> decodePiece(const json& value) {
    if (!value.is_object()) return std::nullopt;
    std::optional<model::PieceKind> kind =
        firstCharMapped(value, kKindKey, io::kindFromLetter);
    std::optional<model::Color> color =
        firstCharMapped(value, kColorKey, io::colorFromLetter);
    if (!kind || !color || !value.contains(kCellKey)) return std::nullopt;
    std::optional<model::Position> cell = decodePosition(value[kCellKey]);
    std::optional<int> code = intField(value, kStateKey);
    std::optional<double> progress = numberField(value, kProgressKey);
    std::optional<int> elapsed = intField(value, kElapsedKey);
    if (!cell || !code || !progress || !elapsed) return std::nullopt;
    std::optional<model::PieceState> state = stateFromCode(*code);
    if (!state) return std::nullopt;

    product::PieceSnapshot piece{*kind, *color,    *cell,     *state,
                                 std::nullopt, *progress, *elapsed};
    if (value.contains(kMovingToKey)) {
        std::optional<model::Position> movingTo =
            decodePosition(value[kMovingToKey]);
        if (!movingTo) return std::nullopt;
        piece.movingTo = *movingTo;
    }
    return piece;
}

// --- move record ---

json encodeMove(const product::MoveRecord& record) {
    return {{kActionKey, actionCode(record.action)},
            {kKindKey, letter(io::kindLetter(record.kind))},
            {kFromKey, encodePosition(record.from)},
            {kToKey, encodePosition(record.to)},
            {kAtMsKey, record.atMs}};
}
std::optional<product::MoveRecord> decodeMove(const json& value) {
    if (!value.is_object()) return std::nullopt;
    std::optional<int> code = intField(value, kActionKey);
    std::optional<model::PieceKind> kind =
        firstCharMapped(value, kKindKey, io::kindFromLetter);
    std::optional<int> atMs = intField(value, kAtMsKey);
    if (!code || !kind || !atMs || !value.contains(kFromKey) ||
        !value.contains(kToKey))
        return std::nullopt;
    std::optional<engine::ActionKind> action = actionFromCode(*code);
    std::optional<model::Position> from = decodePosition(value[kFromKey]);
    std::optional<model::Position> to = decodePosition(value[kToKey]);
    if (!action || !from || !to) return std::nullopt;
    return product::MoveRecord{*action, *kind, *from, *to, *atMs};
}

// --- player ---

json encodePlayer(const product::PlayerSnapshot& player) {
    json moves = json::array();
    for (const product::MoveRecord& record : player.moves)
        moves.push_back(encodeMove(record));
    return {{kColorKey, letter(io::colorLetter(player.color))},
            {kNameKey, player.name},
            {kScoreKey, player.score},
            {kMovesKey, std::move(moves)}};
}
std::optional<product::PlayerSnapshot> decodePlayer(const json& value) {
    if (!value.is_object()) return std::nullopt;
    std::optional<model::Color> color =
        firstCharMapped(value, kColorKey, io::colorFromLetter);
    std::optional<std::string> name = stringField(value, kNameKey);
    std::optional<int> score = intField(value, kScoreKey);
    if (!color || !name || !score || !value.contains(kMovesKey) ||
        !value[kMovesKey].is_array())
        return std::nullopt;

    std::vector<product::MoveRecord> moves;
    for (const json& entry : value[kMovesKey]) {
        std::optional<product::MoveRecord> record = decodeMove(entry);
        if (!record) return std::nullopt;
        moves.push_back(*record);
    }
    return product::PlayerSnapshot{*color, *name, *score, std::move(moves)};
}

}  // namespace

std::string encodeSnapshot(const product::GameStateView& state) {
    json pieces = json::array();
    for (const product::PieceSnapshot& piece : state.pieces)
        pieces.push_back(encodePiece(piece));

    json players = json::array();
    for (const product::PlayerSnapshot& player : state.players)
        players.push_back(encodePlayer(player));

    json out = {{kBoardWidthKey, state.boardWidth},
                {kBoardHeightKey, state.boardHeight},
                {kGameOverKey, state.gameOver},
                {kPiecesKey, std::move(pieces)},
                {kPlayersKey, std::move(players)}};
    // A player's own name is user-supplied text; replace rather than throw if
    // it is not valid UTF-8, so it cannot crash the broadcasting server.
    return out.dump(-1, ' ', false, json::error_handler_t::replace);
}

std::optional<product::GameStateView> decodeSnapshot(const std::string& text) {
    json parsed = json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (!parsed.is_object()) return std::nullopt;

    std::optional<int> width = intField(parsed, kBoardWidthKey);
    std::optional<int> height = intField(parsed, kBoardHeightKey);
    std::optional<bool> gameOver = boolField(parsed, kGameOverKey);
    if (!width || !height || !gameOver || !parsed.contains(kPiecesKey) ||
        !parsed[kPiecesKey].is_array() || !parsed.contains(kPlayersKey) ||
        !parsed[kPlayersKey].is_array())
        return std::nullopt;

    product::GameStateView state;
    state.boardWidth = *width;
    state.boardHeight = *height;
    state.gameOver = *gameOver;

    for (const json& entry : parsed[kPiecesKey]) {
        std::optional<product::PieceSnapshot> piece = decodePiece(entry);
        if (!piece) return std::nullopt;
        state.pieces.push_back(*piece);
    }
    for (const json& entry : parsed[kPlayersKey]) {
        std::optional<product::PlayerSnapshot> player = decodePlayer(entry);
        if (!player) return std::nullopt;
        state.players.push_back(*player);
    }
    return state;
}

}
