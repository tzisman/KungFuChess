#include <cstdint>
#include <string>

#include "common/logger.hpp"
#include "net/transport.hpp"
#include "net/websocketpp_transport.hpp"

namespace {
constexpr std::uint16_t kPort = 9002;
}

// Step 5 server: accepts any number of clients, logs their traffic, and echoes
// each message back. The game is wired in later steps; this proves the transport.
int main() {
    kfc::common::Logger log{"SERVER"};
    kfc::net::WebsocketppServer transport;

    transport.onOpen([&log](kfc::net::ConnectionId id) {
        log.info("client " + std::to_string(id) + " connected");
    });
    transport.onClose([&log](kfc::net::ConnectionId id) {
        log.info("client " + std::to_string(id) + " disconnected");
    });
    transport.onMessage(
        [&log, &transport](kfc::net::ConnectionId id, const std::string& message) {
            log.info("client " + std::to_string(id) + " says: " + message);
            transport.send(id, message);
        });

    log.info("listening on port " + std::to_string(kPort));
    transport.listen(kPort);
    transport.run();
    return 0;
}
