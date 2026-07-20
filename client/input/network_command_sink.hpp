#pragma once

#include "input/command_sink.hpp"
#include "model/position.hpp"
#include "net/transport.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

namespace kfc::input {

// Adapts the network transport to the CommandSink seam for online play: a
// resolved click becomes a MoveIntent/JumpIntent sent to the authoritative
// server, which is the only thing that actually moves a piece. Symmetric with
// EngineCommandSink, but deliberately header-only and left out of the kfc
// library: unlike the engine, net and protocol must never become a dependency
// of the shared Business Logic/GUI library, so only the networked client's
// composition root, which already links them, includes this header.
class NetworkCommandSink : public CommandSink {
public:
    explicit NetworkCommandSink(net::ClientTransport& transport)
        : transport_(transport) {}

    void requestMove(model::Position from, model::Position to) override {
        transport_.send(protocol::encode(protocol::Message{protocol::MoveIntent{from, to}}));
    }
    void requestJump(model::Position cell) override {
        transport_.send(protocol::encode(protocol::Message{protocol::JumpIntent{cell}}));
    }

private:
    net::ClientTransport& transport_;
};

}
