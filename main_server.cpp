#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>

#include "shared/common/logger.hpp"
#include "io/board_parser.hpp"
#include "net/websocketpp_transport.hpp"
#include "realtime/motion.hpp"
#include "server/matchmaker.hpp"
#include "server/room_registry.hpp"
#include "server/scheduler.hpp"
#include "server/server_app.hpp"
#include "server/session_manager.hpp"
#include "server/sqlite_user_store.hpp"

namespace {
constexpr std::uint16_t kPort = 9002;
constexpr char kStartBoardPath[] = "texttests/start.txt";
constexpr char kUserDbPath[] = "kungfu_chess.db";
}

int main() {
    kfc::common::Logger log{"SERVER"};

    std::ifstream startBoard{kStartBoardPath};
    if (!startBoard) {
        std::cerr << "Cannot open " << kStartBoardPath << "\n";
        return 1;
    }
    kfc::io::ParsedInput parsed = kfc::io::parseInput(startBoard);

    kfc::net::WebsocketppServer transport;
    kfc::server::SqliteUserStore users{kUserDbPath};
    kfc::server::SessionManager sessions{transport, users, log};
    kfc::server::Matchmaker matchmaker;
    kfc::server::RoomRegistry rooms{sessions, parsed.board};
    kfc::server::ServerApp app{transport, log, matchmaker, sessions, users, rooms};
    kfc::server::Scheduler scheduler{matchmaker, sessions, transport,
                                     std::move(parsed.board), {}, log};

    transport.listen(kPort);
    log.info("listening on port " + std::to_string(kPort));
    std::thread network([&transport] { transport.run(); });
    std::thread schedulerThread([&scheduler] { scheduler.run(); });

    network.join();  // blocks; the server runs until interrupted
    schedulerThread.join();
    return 0;
}
