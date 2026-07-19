#pragma once

#include "model/position.hpp"

namespace kfc::input {

// The write side of the game as the input layer sees it: where a resolved click
// is sent. Offline this forwards to the engine; over the network it sends an
// intent to the authoritative server. The controller depends only on this seam,
// never on a concrete engine, so the same click handling runs in both modes.
class CommandSink {
public:
    virtual ~CommandSink() = default;

    virtual void requestMove(model::Position from, model::Position to) = 0;
    virtual void requestJump(model::Position cell) = 0;
};

}
