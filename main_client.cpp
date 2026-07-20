#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "app/composition.hpp"
#include "common/logger.hpp"
#include "img.hpp"
#include "input/controller.hpp"
#include "input/network_command_sink.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "net/websocketpp_transport.hpp"
#include "product/game_state_view.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "protocol/wire_snapshot.hpp"
#include "rules/rule_engine.hpp"
#include "view/board_geometry.hpp"
#include "view/game_snapshot.hpp"
#include "view/panel_layout.hpp"
#include "view/renderer.hpp"
#include "view/sprite_library.hpp"
#include "view/window.hpp"

namespace {

constexpr char kServerUri[] = "ws://127.0.0.1:9002";
constexpr char kBoardImagePath[] = "images/board.png";
constexpr char kPiecesRoot[] = "images/pieces3";
constexpr char kWindowTitle[] = "KungFuChess (client)";
constexpr char kDefaultPlayerName[] = "Player";
constexpr int kBoardDisplaySize = 435;
constexpr int kFrameDelayMs = 30;
constexpr int kJoinPollMs = 20;
constexpr int kQuitKey = 27;  // Esc

using Clock = std::chrono::steady_clock;

int elapsedMsSince(Clock::time_point start) {
    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                              start)
            .count());
}

void dispatch(const kfc::view::MouseEvent& event,
              kfc::input::Controller& controller) {
    if (event.type == kfc::view::MouseEvent::Type::kDoubleClick) {
        controller.handleJump(event.x, event.y);
    } else {
        controller.handleClick(event.x, event.y);
    }
}

// Mirrors the wire read-model's pieces into a local Board, so the one existing
// Controller can query it exactly as it does offline. The wire carries no
// piece ids, so a fresh one is handed out per piece here; Controller never
// looks at ids, only at what stands on a cell. A landing piece can briefly
// share its cell with the piece it is displacing, so the first one seen for a
// cell wins and later duplicates are dropped.
void syncBoard(kfc::model::Board& board,
              const std::vector<kfc::product::PieceSnapshot>& pieces) {
    board.clear();
    kfc::model::PieceId id = 0;
    for (const kfc::product::PieceSnapshot& piece : pieces) {
        if (board.pieceAt(piece.cell)) continue;
        board.addPiece(
            kfc::model::Piece{id++, piece.color, piece.kind, piece.cell, piece.state});
    }
}

// Printable ASCII only. The panel draws names with OpenCV's built-in Hershey
// font, which has no glyphs outside that range, and a name typed in the
// console's native codepage is not guaranteed to be valid UTF-8 either — both
// are sidestepped by asking again rather than sending text that cannot be
// shown or safely encoded.
bool isPlainAscii(const std::string& text) {
    for (unsigned char c : text) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

// The same legality the offline path gets from GameEngine::legalDestinationsFrom
// (rules::RuleEngine plus an idle, game-not-over guard), computed against the
// client's own local mirror instead of an engine it does not have. This is a
// display hint only: the server enforces the real rule when the move actually
// arrives, so a stale frame here costs nothing but a momentarily wrong dot.
std::vector<kfc::model::Position> legalDestinationsFrom(
    const kfc::model::Board& board, kfc::model::Position from, bool gameOver) {
    if (gameOver) return {};
    std::optional<kfc::model::Piece> piece = board.pieceAt(from);
    if (!piece || piece->state() != kfc::model::PieceState::kIdle) return {};

    kfc::rules::RuleEngine ruleEngine;
    kfc::rules::Destinations destinations = ruleEngine.legalDestinations(board, from);
    return {destinations.begin(), destinations.end()};
}

std::string askForName() {
    while (true) {
        std::cout << "Enter your name (English letters/digits only): ";
        std::string name;
        std::getline(std::cin, name);
        if (name.empty()) return kDefaultPlayerName;
        if (isPlainAscii(name)) return name;
        std::cout << "Please use plain English letters and digits.\n";
    }
}

// What joining resolved to, filled in from the network thread's onMessage/
// onClose handlers and read by the main thread's wait loop below. Only one of
// the three ever becomes set.
struct JoinOutcome {
    std::mutex mutex;
    std::optional<kfc::model::Color> color;
    std::optional<std::string> rejection;
    bool closed = false;
};

// Blocks the caller until the server has answered the join request one way or
// another, so the window never opens on a rejected or dropped connection. A
// short poll is enough: this only runs once, before the game itself starts.
std::optional<kfc::model::Color> awaitJoin(JoinOutcome& outcome) {
    while (true) {
        {
            std::lock_guard<std::mutex> lock{outcome.mutex};
            if (outcome.color) return outcome.color;
            if (outcome.rejection) {
                std::cerr << "Server rejected join: " << *outcome.rejection << "\n";
                return std::nullopt;
            }
            if (outcome.closed) {
                std::cerr << "Connection closed before joining\n";
                return std::nullopt;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kJoinPollMs));
    }
}

}  // namespace

// Step 11 client: registers by name in the terminal and opens the game window
// only once the server has assigned a colour, completing the join flow from
// step 2 of the plan. From there it plays exactly as step 10 left it: no local
// engine, every click becomes a MoveIntent/JumpIntent, every frame drawn comes
// from the server's own broadcast snapshot. The transport's io loop runs on a
// background thread; the main thread owns OpenCV and the shared state it reads
// is guarded by a mutex throughout.
int main() {
    kfc::common::Logger log{"CLIENT"};
    std::string name = askForName();

    JoinOutcome join;
    std::mutex stateMutex;
    std::optional<kfc::product::GameStateView> latest;

    kfc::net::WebsocketppClient transport;
    kfc::input::NetworkCommandSink commands{transport};

    transport.onOpen([&log, &transport, &name]() {
        log.info("connected -> joining as " + name);
        transport.send(kfc::protocol::encode(
            kfc::protocol::Message{kfc::protocol::JoinRequest{name}}));
    });
    transport.onClose([&log, &join]() {
        log.info("closed");
        std::lock_guard<std::mutex> lock{join.mutex};
        join.closed = true;
    });
    transport.onMessage([&join, &stateMutex, &latest](const std::string& text) {
        if (std::optional<kfc::protocol::Message> message =
                kfc::protocol::decode(text)) {
            if (const auto* assigned =
                    std::get_if<kfc::protocol::Assigned>(&*message)) {
                std::lock_guard<std::mutex> lock{join.mutex};
                join.color = assigned->color;
            } else if (const auto* rejected =
                           std::get_if<kfc::protocol::Rejected>(&*message)) {
                std::lock_guard<std::mutex> lock{join.mutex};
                join.rejection = rejected->reason;
            }
            return;
        }
        std::optional<kfc::product::GameStateView> decoded =
            kfc::protocol::decodeSnapshot(text);
        if (!decoded) return;
        std::lock_guard<std::mutex> lock{stateMutex};
        latest = std::move(decoded);
    });

    log.info(std::string{"connecting to "} + kServerUri);
    transport.connect(kServerUri);
    std::thread network([&transport] { transport.run(); });

    std::optional<kfc::model::Color> myColor = awaitJoin(join);
    if (!myColor) {
        transport.stop();
        network.join();
        return 1;
    }

    Img boardImage;
    boardImage.read(kBoardImagePath, {kBoardDisplaySize, kBoardDisplaySize},
                    /*keep_aspect=*/true);
    kfc::view::PanelLayout layout{boardImage.get_mat().cols,
                                  boardImage.get_mat().rows};
    kfc::view::Window window{kWindowTitle};

    // Built lazily once the board's dimensions are known from the first
    // snapshot; myColor is already in hand by the time we get here.
    std::optional<kfc::view::SpriteLibrary> sprites;
    std::optional<kfc::view::Renderer> renderer;
    std::optional<kfc::model::Board> board;
    std::optional<kfc::input::Controller> controller;

    Clock::time_point start = Clock::now();
    while (true) {
        std::optional<kfc::product::GameStateView> current;
        {
            std::lock_guard<std::mutex> lock{stateMutex};
            current = latest;
        }

        if (current) {
            if (!renderer) {
                kfc::view::BoardGeometry geometry{
                    boardImage.get_mat().cols, boardImage.get_mat().rows,
                    current->boardWidth, current->boardHeight,
                    layout.boardOrigin()};
                sprites.emplace(kPiecesRoot, geometry.cellWidth(),
                                geometry.cellHeight());
                renderer.emplace(kBoardImagePath, *sprites, geometry, layout);
                board.emplace(current->boardWidth, current->boardHeight);
                controller.emplace(kfc::app::makeController(*board, commands,
                                                            geometry, myColor));
            }
            syncBoard(*board, current->pieces);

            for (const kfc::view::MouseEvent& event : window.takeMouseEvents()) {
                dispatch(event, *controller);
            }

            std::vector<kfc::model::Position> moveTargets;
            if (std::optional<kfc::model::Position> selection = controller->selection()) {
                moveTargets =
                    legalDestinationsFrom(*board, *selection, current->gameOver);
            }

            window.show(renderer->render(
                kfc::view::buildSnapshot(std::move(*current),
                                         controller->selection(),
                                         std::move(moveTargets)),
                elapsedMsSince(start)));
        }

        if (window.waitKey(kFrameDelayMs) == kQuitKey) break;
    }

    transport.stop();
    network.join();
    return 0;
}
