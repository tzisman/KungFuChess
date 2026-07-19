#include <string>

#include "common/logger.hpp"
#include "net/transport.hpp"
#include "net/websocketpp_transport.hpp"

namespace {
constexpr char kServerUri[] = "ws://127.0.0.1:9002";
}

// Step 6 client: connects to the server, sends one line, prints the echo, and
// exits. The GUI and the game are wired in later steps; this proves the client
// side of the transport talks to the server over loopback.
int main(int argc, char** argv) {
    std::string message = argc > 1 ? argv[1] : "hello-from-client";

    kfc::common::Logger log{"CLIENT"};
    kfc::net::WebsocketppClient transport;

    transport.onOpen([&log, &transport, &message]() {
        log.info("connected -> sending: " + message);
        transport.send(message);
    });
    transport.onMessage([&log, &transport](const std::string& reply) {
        log.info("received: " + reply);
        transport.stop();
    });
    transport.onClose([&log]() { log.info("closed"); });

    log.info(std::string{"connecting to "} + kServerUri);
    transport.connect(kServerUri);
    transport.run();
    return 0;
}
