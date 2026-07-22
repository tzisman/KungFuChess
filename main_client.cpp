#include <optional>
#include <string>
#include <thread>

#include "app/auth_flow.hpp"
#include "app/client_app.hpp"
#include "app/network_session.hpp"
#include "common/logger.hpp"
#include "input/network_command_sink.hpp"
#include "input/network_lobby_sink.hpp"
#include "net/websocketpp_transport.hpp"

namespace {
constexpr char kServerUri[] = "ws://127.0.0.1:9002";
}

int main() {
    kfc::common::Logger log{"CLIENT"};

    kfc::net::WebsocketppClient transport;
    kfc::app::NetworkSession session{transport, log};
    kfc::input::NetworkCommandSink commands{transport};
    kfc::input::NetworkLobbySink lobbySink{transport};

    log.info(std::string{"connecting to "} + kServerUri);
    transport.connect(kServerUri);
    std::thread network([&transport] { transport.run(); });

    kfc::app::loginInteractively(transport, log, session);

    kfc::app::ClientApp app{session, commands, lobbySink, log, session.loginRating()};
    app.run();

    transport.stop();
    network.join();
    return 0;
}
