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

    void requestEnterRoom(const std::string& roomName) {
        transport_.send(
            protocol::encode(protocol::Message{protocol::EnterRoomRequest{roomName}}));
    }

private:
    net::ClientTransport& transport_;
};

}
