#include <doctest/doctest.h>

#include "bus/event_bus.hpp"
#include "engine/game_events.hpp"
#include "model/piece.hpp"
#include "model/piece_cost.hpp"
#include "model/position.hpp"
#include "product/score_board.hpp"

using kfc::engine::CaptureEvent;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::product::ScoreBoard;

namespace {

constexpr int kAnyId = 1;
constexpr int kAnyTime = 0;
constexpr Position kAnyCell{4, 4};

CaptureEvent captureOf(PieceKind kind, Color victimColor, Color capturedBy,
                       int atMs = kAnyTime) {
    return {Piece{kAnyId, victimColor, kind, kAnyCell}, capturedBy, atMs};
}

}  // namespace

TEST_CASE("a fresh scoreboard has nothing on it") {
    ScoreBoard scores;

    CHECK(scores.scoreOf(Color::kWhite) == 0);
    CHECK(scores.scoreOf(Color::kBlack) == 0);
}

TEST_CASE("a capture scores the victim's cost to the captor") {
    ScoreBoard scores;

    scores.onCapture(captureOf(PieceKind::kQueen, Color::kBlack, Color::kWhite));

    CHECK(scores.scoreOf(Color::kWhite) == kfc::model::costOf(PieceKind::kQueen));
    CHECK(scores.scoreOf(Color::kBlack) == 0);
}

TEST_CASE("captures accumulate for each player separately") {
    ScoreBoard scores;

    scores.onCapture(captureOf(PieceKind::kPawn, Color::kBlack, Color::kWhite));
    scores.onCapture(captureOf(PieceKind::kRook, Color::kBlack, Color::kWhite));
    scores.onCapture(captureOf(PieceKind::kKnight, Color::kWhite, Color::kBlack));

    CHECK(scores.scoreOf(Color::kWhite) ==
          kfc::model::costOf(PieceKind::kPawn) +
              kfc::model::costOf(PieceKind::kRook));
    CHECK(scores.scoreOf(Color::kBlack) == kfc::model::costOf(PieceKind::kKnight));
}

TEST_CASE("a subscribed scoreboard counts captures the bus delivers") {
    kfc::bus::EventBus bus;
    ScoreBoard scores;
    scores.subscribeTo(bus);

    bus.publish(captureOf(PieceKind::kQueen, Color::kBlack, Color::kWhite));

    CHECK(scores.scoreOf(Color::kWhite) == kfc::model::costOf(PieceKind::kQueen));
}

TEST_CASE("a scoreboard ignores what it is not there to count") {
    kfc::bus::EventBus bus;
    ScoreBoard scores;
    scores.subscribeTo(bus);

    bus.publish(kfc::engine::ActionEvent{kAnyId, Color::kWhite, PieceKind::kRook,
                                         kAnyCell, kAnyCell,
                                         kfc::engine::ActionKind::kMove, kAnyTime});
    bus.publish(kfc::engine::GameOverEvent{Color::kWhite, kAnyTime});

    CHECK(scores.scoreOf(Color::kWhite) == 0);
}
