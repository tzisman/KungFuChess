#include <doctest/doctest.h>

#include <vector>

#include "bus/event_bus.hpp"
#include "engine/game_engine.hpp"
#include "engine/game_events.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "model/position.hpp"
#include "realtime/motion.hpp"

using kfc::engine::ActionEvent;
using kfc::engine::ActionKind;
using kfc::engine::CaptureEvent;
using kfc::engine::GameEngine;
using kfc::engine::GameOverEvent;
using kfc::engine::MoveResult;
using kfc::model::Board;
using kfc::model::Color;
using kfc::model::Piece;
using kfc::model::PieceKind;
using kfc::model::Position;
using kfc::realtime::kLongRestMs;
using kfc::realtime::kSquareTravelMs;

namespace {

Board boardWith(std::initializer_list<Piece> pieces) {
    Board board{8, 8};
    for (const Piece& piece : pieces) {
        board.addPiece(piece);
    }
    return board;
}

// Stands in for anything watching the game, and remembers what it was told so
// the telling itself can be checked.
class RecordingObserver {
public:
    void subscribeTo(kfc::bus::EventBus& bus) {
        bus.subscribe<ActionEvent>([this](const ActionEvent& e) { actions.push_back(e); });
        bus.subscribe<CaptureEvent>([this](const CaptureEvent& e) { captures.push_back(e); });
        bus.subscribe<GameOverEvent>([this](const GameOverEvent& e) { overs.push_back(e); });
    }

    std::vector<ActionEvent> actions;
    std::vector<CaptureEvent> captures;
    std::vector<GameOverEvent> overs;
};

}

TEST_CASE("a legal move is accepted and reported as ok") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};

    MoveResult result = engine.requestMove(Position{4, 4}, Position{4, 7});

    CHECK(result.isAccepted);
    CHECK(result.reason == "ok");
}

TEST_CASE("an illegal piece move carries the rule-level reason") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};

    MoveResult result = engine.requestMove(Position{4, 4}, Position{5, 5});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "illegal_piece_move");
}

TEST_CASE("moving from an empty source carries the rule-level reason") {
    GameEngine engine{Board{8, 8}};

    MoveResult result = engine.requestMove(Position{0, 0}, Position{0, 3});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "empty_source");
}

TEST_CASE("an idle piece may move while another is already in motion") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kWhite, PieceKind::kRook, Position{0, 0}},
    })};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    MoveResult result = engine.requestMove(Position{0, 0}, Position{0, 3});

    CHECK(result.isAccepted);
}

TEST_CASE("a piece already in motion cannot be commanded again") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    MoveResult result = engine.requestMove(Position{4, 4}, Position{4, 5});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "not_idle");
}

TEST_CASE("a piece that just arrived is resting and cannot move yet") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    engine.advance(3 * kSquareTravelMs);

    MoveResult result = engine.requestMove(Position{4, 7}, Position{4, 4});
    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "not_idle");
}

TEST_CASE("a new move is allowed once the cooldown after arrival elapses") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    engine.advance(3 * kSquareTravelMs);
    engine.advance(kLongRestMs);

    CHECK(engine.requestMove(Position{4, 7}, Position{4, 4}).isAccepted);
}

TEST_CASE("wait advances the board through the arbiter") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    engine.advance(3 * kSquareTravelMs);

    CHECK_FALSE(engine.board().pieceAt(Position{4, 4}).has_value());
    CHECK(engine.board().pieceAt(Position{4, 7}).has_value());
}

TEST_CASE("capturing the king ends the game") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 6}},
    })};
    engine.requestMove(Position{4, 4}, Position{4, 6});

    engine.advance(2 * kSquareTravelMs);

    CHECK(engine.isOver());
}

TEST_CASE("moves are rejected once the game is over") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 6}},
    })};
    engine.requestMove(Position{4, 4}, Position{4, 6});
    engine.advance(2 * kSquareTravelMs);

    MoveResult result = engine.requestMove(Position{4, 6}, Position{4, 5});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "game_over");
}

TEST_CASE("a pawn reaching the last row is promoted to a queen") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kPawn, Position{1, 4}}})};
    engine.requestMove(Position{1, 4}, Position{0, 4});

    engine.advance(kSquareTravelMs);

    auto piece = engine.board().pieceAt(Position{0, 4});
    REQUIRE(piece.has_value());
    CHECK(piece->kind() == PieceKind::kQueen);
}

TEST_CASE("a jump on an idle piece is accepted") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK(result.isAccepted);
    CHECK(result.reason == "ok");
}

TEST_CASE("a jump on an empty cell is rejected") {
    GameEngine engine{Board{8, 8}};

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "no_piece");
}

TEST_CASE("a moving piece cannot jump") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 7});

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "not_idle");
}

TEST_CASE("an airborne piece cannot jump again") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestJump(Position{4, 4});

    MoveResult result = engine.requestJump(Position{4, 4});

    CHECK_FALSE(result.isAccepted);
    CHECK(result.reason == "not_idle");
}

TEST_CASE("an airborne piece capturing an arriving king ends the game") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 5}},
    })};
    engine.requestJump(Position{4, 4});
    engine.requestMove(Position{4, 5}, Position{4, 4});

    engine.advance(kSquareTravelMs);

    CHECK(engine.isOver());
    CHECK(engine.board().pieceAt(Position{4, 4})->kind() == PieceKind::kRook);
}

TEST_CASE("the engine offers exactly the destinations it would accept a move to") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kKing, Position{4, 4}},
        Piece{2, Color::kWhite, PieceKind::kRook, Position{4, 5}},
        Piece{3, Color::kBlack, PieceKind::kRook, Position{3, 4}},
    })};

    auto destinations = engine.legalDestinationsFrom(Position{4, 4});

    CHECK(destinations.count(Position{3, 4}) == 1);
    CHECK(destinations.count(Position{5, 4}) == 1);
    CHECK(destinations.count(Position{4, 5}) == 0);
    CHECK(destinations.count(Position{4, 6}) == 0);
}

TEST_CASE("an empty cell offers no destinations") {
    GameEngine engine{Board{8, 8}};

    CHECK(engine.legalDestinationsFrom(Position{4, 4}).empty());
}

TEST_CASE("a resting piece offers no destinations to highlight") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    engine.requestMove(Position{4, 4}, Position{4, 5});
    engine.advance(kSquareTravelMs);

    CHECK(engine.legalDestinationsFrom(Position{4, 5}).empty());
}

TEST_CASE("an accepted move is reported to whoever is watching") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());

    engine.requestMove(Position{4, 4}, Position{4, 7});

    REQUIRE(observer.actions.size() == 1);
    CHECK(observer.actions[0].color == Color::kWhite);
    CHECK(observer.actions[0].kind == PieceKind::kRook);
    CHECK(observer.actions[0].from == Position{4, 4});
    CHECK(observer.actions[0].to == Position{4, 7});
    CHECK(observer.actions[0].action == ActionKind::kMove);
}

TEST_CASE("a rejected move is reported to nobody") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());

    engine.requestMove(Position{4, 4}, Position{5, 5});

    CHECK(observer.actions.empty());
}

TEST_CASE("an accepted jump is reported as a jump that goes nowhere") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());

    engine.requestJump(Position{4, 4});

    REQUIRE(observer.actions.size() == 1);
    CHECK(observer.actions[0].action == ActionKind::kJump);
    CHECK(observer.actions[0].from == Position{4, 4});
    CHECK(observer.actions[0].to == Position{4, 4});
}

TEST_CASE("an action is stamped with the time on the game clock") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());

    engine.advance(kSquareTravelMs);
    engine.requestMove(Position{4, 4}, Position{4, 7});

    REQUIRE(observer.actions.size() == 1);
    CHECK(observer.actions[0].atMs == kSquareTravelMs);
}

TEST_CASE("a capture names the victim and the piece that took it") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kQueen, Position{4, 6}},
    })};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());
    engine.requestMove(Position{4, 4}, Position{4, 6});

    engine.advance(2 * kSquareTravelMs);

    REQUIRE(observer.captures.size() == 1);
    CHECK(observer.captures[0].victim.kind() == PieceKind::kQueen);
    CHECK(observer.captures[0].victim.color() == Color::kBlack);
    CHECK(observer.captures[0].capturedBy == Color::kWhite);
}

TEST_CASE("a move onto an empty square captures nothing") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());
    engine.requestMove(Position{4, 4}, Position{4, 7});

    engine.advance(3 * kSquareTravelMs);

    CHECK(observer.captures.empty());
}

// The win must not swallow the capture that won it: a scoreboard learns of the
// king the same way it learns of any other piece.
TEST_CASE("taking the king is reported as a capture before the game is called") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 6}},
    })};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());
    engine.requestMove(Position{4, 4}, Position{4, 6});

    engine.advance(2 * kSquareTravelMs);

    REQUIRE(observer.captures.size() == 1);
    CHECK(observer.captures[0].victim.kind() == PieceKind::kKing);
    REQUIRE(observer.overs.size() == 1);
    CHECK(observer.overs[0].winner == Color::kWhite);
}

TEST_CASE("a landing jump that takes the king names the jumper as the winner") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 5}},
    })};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());
    engine.requestJump(Position{4, 4});
    engine.requestMove(Position{4, 5}, Position{4, 4});

    engine.advance(kSquareTravelMs);

    REQUIRE(observer.captures.size() == 1);
    CHECK(observer.captures[0].victim.kind() == PieceKind::kKing);
    CHECK(observer.captures[0].capturedBy == Color::kWhite);
    REQUIRE(observer.overs.size() == 1);
    CHECK(observer.overs[0].winner == Color::kWhite);
}

TEST_CASE("declareForfeit ends the game and publishes a GameOverEvent for the given winner") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());

    engine.declareForfeit(Color::kBlack);

    CHECK(engine.isOver());
    REQUIRE(observer.overs.size() == 1);
    CHECK(observer.overs[0].winner == Color::kBlack);
}

TEST_CASE("declareForfeit on a game that already ended is a no-op") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver observer;
    observer.subscribeTo(engine.events());
    engine.declareForfeit(Color::kBlack);

    engine.declareForfeit(Color::kWhite);

    CHECK(observer.overs.size() == 1);
    CHECK(observer.overs[0].winner == Color::kBlack);
}

TEST_CASE("every observer hears the same events") {
    GameEngine engine{boardWith({Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}}})};
    RecordingObserver first;
    RecordingObserver second;
    first.subscribeTo(engine.events());
    second.subscribeTo(engine.events());

    engine.requestMove(Position{4, 4}, Position{4, 7});

    CHECK(first.actions.size() == 1);
    CHECK(second.actions.size() == 1);
}

TEST_CASE("a game nobody is watching runs just the same") {
    GameEngine engine{boardWith({
        Piece{1, Color::kWhite, PieceKind::kRook, Position{4, 4}},
        Piece{2, Color::kBlack, PieceKind::kKing, Position{4, 6}},
    })};

    engine.requestMove(Position{4, 4}, Position{4, 6});
    engine.advance(2 * kSquareTravelMs);

    CHECK(engine.isOver());
}
