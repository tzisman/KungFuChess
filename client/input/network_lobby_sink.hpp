#pragma once

#include <string>

#include "net/transport.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

namespace kfc::input {

// Adapts the network transport to the lobby's actions. Header-only and left
// out of the kfc library for the same reason as NetworkCommandSink: only the
// networked client's composition root links net/protocol.
class NetworkLobbySink {
public:
    explicit NetworkLobbySink(net::ClientTransport& transport) : transport_(transport) {}

    void requestPlay() {
        transport_.send(protocol::encode(protocol::Message{protocol::PlayRequest{}}));
    }

    // Rooms (Phase D) have no wire message yet, so entering one is a no-op
    // for now — the lobby button is wired ahead of the protocol it will use.
    void requestEnterRoom(const std::string& roomName) { (void)roomName; }

private:
    net::ClientTransport& transport_;
};

}
