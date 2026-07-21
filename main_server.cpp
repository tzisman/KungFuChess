#include <cstdint>
#include <thread>

#include "shared/common/logger.hpp"
#include "net/websocketpp_transport.hpp"
#include "server/command_queue.hpp"
#include "server/server_app.hpp"
#include "server/session_manager.hpp"
#include "server/sqlite_user_store.hpp"

namespace {
constexpr std::uint16_t kPort = 9002;
constexpr char kUserDbPath[] = "kungfu_chess.db";
}

// Composition root of the server. It owns the concrete transport, the join
// logic (ServerApp), and the SessionManager that will hold whatever concurrent
// matches exist. Nothing seats a player into a match yet — that arrives with
// matchmaking — so the main thread simply blocks on the network event loop.
int main() {
    kfc::common::Logger log{"SERVER"};

    kfc::net::WebsocketppServer transport;
    kfc::server::CommandQueue commands;
    kfc::server::SqliteUserStore users{kUserDbPath};
    kfc::server::ServerApp app{transport, log, commands, users};
    kfc::server::SessionManager sessions{transport, users, log};

    transport.listen(kPort);
    log.info("listening on port " + std::to_string(kPort));
    std::thread network([&transport] { transport.run(); });

    network.join();  // blocks; the server runs until interrupted
    return 0;
}
