#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>

#include "shared/common/logger.hpp"
#include "io/board_parser.hpp"
#include "net/websocketpp_transport.hpp"
#include "server/command_queue.hpp"
#include "server/game_session.hpp"
#include "server/player_names.hpp"
#include "server/server_app.hpp"

namespace {
constexpr std::uint16_t kPort = 9002;
constexpr char kStartBoardPath[] = "texttests/start.txt";
}

// Composition root of the server. It owns the concrete transport, the join logic
// (ServerApp) and the authoritative game (GameSession). The network event loop
// runs on its own thread; the game loop — the only thread that touches the
// engine — runs on the main thread.
int main() {
    kfc::common::Logger log{"SERVER"};

    std::ifstream startBoard{kStartBoardPath};
    if (!startBoard) {
        std::cerr << "Cannot open " << kStartBoardPath << "\n";
        return 1;
    }
    kfc::io::ParsedInput parsed = kfc::io::parseInput(startBoard);

    kfc::net::WebsocketppServer transport;
    kfc::server::CommandQueue commands;
    kfc::server::PlayerNames names;
    kfc::server::ServerApp app{transport, log, commands, names};
    kfc::server::GameSession game{transport, commands, names,
                                  std::move(parsed.board)};

    transport.listen(kPort);
    log.info("listening on port " + std::to_string(kPort));
    std::thread network([&transport] { transport.run(); });

    game.run();  // blocks; the server runs until interrupted

    transport.stop();
    network.join();
    return 0;
}
