#include <cstdint>

#include "common/logger.hpp"
#include "net/websocketpp_transport.hpp"
#include "server/server_app.hpp"

namespace {
constexpr std::uint16_t kPort = 9002;
}

// Composition root of the server: wires the concrete transport to the server
// application and runs it. The first two players to join are assigned white and
// black; further joiners are turned away.
int main() {
    kfc::common::Logger log{"SERVER"};
    kfc::net::WebsocketppServer transport;
    kfc::server::ServerApp app{transport, log};
    app.start(kPort);
    return 0;
}
