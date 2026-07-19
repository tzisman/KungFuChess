#include <doctest/doctest.h>

#include "bus/event_bus.hpp"
#include "engine/game_events.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "product/move_log.hpp"

using kfc::engine::ActionEvent;
using kfc::engine::ActionKind;
using kfc::model::Color;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::product::MoveLog;

namespace {

constexpr int kAnyId = 1;

ActionEvent moveOf(Color color, PieceKind kind, Position from, Position to,
                   int atMs) {
    return {kAnyId, color, kind, from, to, ActionKind::kMove, atMs};
}

}  // namespace

TEST_CASE("a fresh log holds nothing for either player") {
    MoveLog log;

    CHECK(log.movesOf(Color::kWhite).empty());
    CHECK(log.movesOf(Color::kBlack).empty());
}

TEST_CASE("an action is logged as the player's own") {
    MoveLog log;

    log.onAction(moveOf(Color::kWhite, PieceKind::kPawn, Position{6, 4},
                        Position{4, 4}, 2314));

    REQUIRE(log.movesOf(Color::kWhite).size() == 1);
    CHECK(log.movesOf(Color::kBlack).empty());

    const kfc::product::MoveRecord& record = log.movesOf(Color::kWhite).front();
    CHECK(record.action == ActionKind::kMove);
    CHECK(record.kind == PieceKind::kPawn);
    CHECK(record.from == Position{6, 4});
    CHECK(record.to == Position{4, 4});
    CHECK(record.atMs == 2314);
}

TEST_CASE("each player's actions are kept apart and in the order they happened") {
    MoveLog log;

    log.onAction(moveOf(Color::kWhite, PieceKind::kPawn, Position{6, 4},
                        Position{4, 4}, 1000));
    log.onAction(moveOf(Color::kBlack, PieceKind::kKnight, Position{0, 1},
                        Position{2, 2}, 1500));
    log.onAction(moveOf(Color::kWhite, PieceKind::kKnight, Position{7, 6},
                        Position{5, 5}, 2000));

    REQUIRE(log.movesOf(Color::kWhite).size() == 2);
    REQUIRE(log.movesOf(Color::kBlack).size() == 1);
    CHECK(log.movesOf(Color::kWhite)[0].atMs == 1000);
    CHECK(log.movesOf(Color::kWhite)[1].atMs == 2000);
    CHECK(log.movesOf(Color::kBlack)[0].kind == PieceKind::kKnight);
}

TEST_CASE("a jump is logged as a jump, going nowhere") {
    MoveLog log;

    log.onAction({kAnyId, Color::kBlack, PieceKind::kRook, Position{3, 3},
                  Position{3, 3}, ActionKind::kJump, 500});

    REQUIRE(log.movesOf(Color::kBlack).size() == 1);
    const kfc::product::MoveRecord& record = log.movesOf(Color::kBlack).front();
    CHECK(record.action == ActionKind::kJump);
    CHECK(record.from == record.to);
}

TEST_CASE("a subscribed log records the actions the bus delivers") {
    kfc::bus::EventBus bus;
    MoveLog log;
    log.subscribeTo(bus);

    bus.publish(moveOf(Color::kWhite, PieceKind::kPawn, Position{6, 4},
                       Position{4, 4}, 2314));

    REQUIRE(log.movesOf(Color::kWhite).size() == 1);
    CHECK(log.movesOf(Color::kWhite).front().atMs == 2314);
}

TEST_CASE("a log ignores what it is not there to record") {
    kfc::bus::EventBus bus;
    MoveLog log;
    log.subscribeTo(bus);

    bus.publish(kfc::engine::CaptureEvent{
        kfc::model::Piece{kAnyId, Color::kBlack, PieceKind::kPawn, Position{4, 4}},
        Color::kWhite, 100});
    bus.publish(kfc::engine::GameOverEvent{Color::kWhite, 200});

    CHECK(log.movesOf(Color::kWhite).empty());
    CHECK(log.movesOf(Color::kBlack).empty());
}
