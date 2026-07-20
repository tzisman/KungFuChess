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

}
