#include <doctest/doctest.h>

#include <optional>
#include <utility>

#include "engine/game_engine.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "product/game_state_view.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"
#include "protocol/wire_snapshot.hpp"
#include "realtime/motion.hpp"

using kfc::engine::GameEngine;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::product::GameStateView;
using kfc::product::MoveLog;
using kfc::product::PlayerSnapshot;
using kfc::product::ScoreBoard;
using kfc::protocol::decodeSnapshot;
using kfc::protocol::encodeSnapshot;
using kfc::realtime::kSquareTravelMs;

namespace {

const PlayerSnapshot& playerOf(const GameStateView& state, Color color) {
    for (const PlayerSnapshot& player : state.players)
        if (player.color == color) return player;
    FAIL("no such player");
    return state.players.front();
}

// A full field-by-field check that decoding reproduces what was encoded.
void checkSame(const GameStateView& a, const GameStateView& b) {
    CHECK(a.boardWidth == b.boardWidth);
    CHECK(a.boardHeight == b.boardHeight);
    CHECK(a.gameOver == b.gameOver);

    REQUIRE(a.pieces.size() == b.pieces.size());
    for (size_t i = 0; i < a.pieces.size(); ++i) {
        CHECK(a.pieces[i].kind == b.pieces[i].kind);
        CHECK(a.pieces[i].color == b.pieces[i].color);
        CHECK(a.pieces[i].cell == b.pieces[i].cell);
        CHECK(a.pieces[i].state == b.pieces[i].state);
        CHECK(a.pieces[i].movingTo.has_value() == b.pieces[i].movingTo.has_value());
        if (a.pieces[i].movingTo)
            CHECK(*a.pieces[i].movingTo == *b.pieces[i].movingTo);
        CHECK(a.pieces[i].progress == doctest::Approx(b.pieces[i].progress));
        CHECK(a.pieces[i].stateElapsedMs == b.pieces[i].stateElapsedMs);
    }

    REQUIRE(a.players.size() == b.players.size());
    for (size_t i = 0; i < a.players.size(); ++i) {
        CHECK(a.players[i].color == b.players[i].color);
        CHECK(a.players[i].name == b.players[i].name);
        CHECK(a.players[i].score == b.players[i].score);
        REQUIRE(a.players[i].moves.size() == b.players[i].moves.size());
        for (size_t j = 0; j < a.players[i].moves.size(); ++j) {
            CHECK(a.players[i].moves[j].action == b.players[i].moves[j].action);
            CHECK(a.players[i].moves[j].kind == b.players[i].moves[j].kind);
            CHECK(a.players[i].moves[j].from == b.players[i].moves[j].from);
            CHECK(a.players[i].moves[j].to == b.players[i].moves[j].to);
            CHECK(a.players[i].moves[j].atMs == b.players[i].moves[j].atMs);
        }
    }
}

}  // namespace

TEST_CASE("an empty snapshot round-trips") {
    GameEngine engine{Board{8, 8}};
    GameStateView original = kfc::product::gameStateView(engine, ScoreBoard{}, MoveLog{});

    std::optional<GameStateView> decoded = decodeSnapshot(encodeSnapshot(original));

    REQUIRE(decoded.has_value());
    checkSame(original, *decoded);
    CHECK(decoded->pieces.empty());
}

TEST_CASE("a snapshot with a moving piece round-trips its motion") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    GameEngine engine{std::move(board)};
    engine.requestMove(Position{4, 4}, Position{4, 6});
    engine.advance(3 * kSquareTravelMs / 2);

    GameStateView original = kfc::product::gameStateView(engine, ScoreBoard{}, MoveLog{});
    std::optional<GameStateView> decoded = decodeSnapshot(encodeSnapshot(original));

    REQUIRE(decoded.has_value());
    checkSame(original, *decoded);
    REQUIRE(decoded->pieces.size() == 1);
    CHECK(decoded->pieces.front().movingTo.has_value());
    CHECK(decoded->pieces.front().progress == doctest::Approx(0.5));
}

TEST_CASE("a snapshot with a capture round-trips scores and move records") {
    Board board{8, 8};
    board.addPiece(Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}});
    board.addPiece(Piece{2, Color::kBlack, PieceKind::kQueen, Position{4, 6}});
    GameEngine engine{std::move(board)};
    ScoreBoard scores;
    MoveLog log;
    scores.subscribeTo(engine.events());
    log.subscribeTo(engine.events());

    engine.requestMove(Position{4, 4}, Position{4, 6});
    engine.advance(2 * kSquareTravelMs);

    GameStateView original = kfc::product::gameStateView(engine, scores, log);
    std::optional<GameStateView> decoded = decodeSnapshot(encodeSnapshot(original));

    REQUIRE(decoded.has_value());
    checkSame(original, *decoded);
    CHECK(playerOf(*decoded, Color::kWhite).score == 9);
    REQUIRE(playerOf(*decoded, Color::kWhite).moves.size() == 1);
    CHECK(playerOf(*decoded, Color::kWhite).moves.front().to == Position{4, 6});
}

TEST_CASE("decoding a snapshot rejects malformed input") {
    CHECK_FALSE(decodeSnapshot("not json").has_value());
    CHECK_FALSE(decodeSnapshot("{}").has_value());
    CHECK_FALSE(decodeSnapshot(R"({"boardWidth":8})").has_value());
}
