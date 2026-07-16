#pragma once

#include "model/piece.hpp"
#include "model/position.hpp"

namespace kfc::engine {

enum class ActionKind { kMove, kJump };

// What a player commanded, reported the moment the engine accepted it. A jump
// stays where it is, so its from and to are the same square.
struct ActionEvent {
    model::PieceId pieceId;
    model::Color color;
    model::PieceKind kind;
    model::Position from;
    model::Position to;
    ActionKind action;
    int atMs;
};

struct CaptureEvent {
    model::Piece victim;
    model::Color capturedBy;
    int atMs;
};

struct GameOverEvent {
    model::Color winner;
    int atMs;
};

// Anything that wants to follow the game without the game knowing it exists.
// Every handler does nothing by default, so an observer overrides only the
// events it actually cares about.
class GameObserver {
public:
    virtual ~GameObserver() = default;

    virtual void onAction(const ActionEvent&) {}
    virtual void onCapture(const CaptureEvent&) {}
    virtual void onGameOver(const GameOverEvent&) {}
};

}
