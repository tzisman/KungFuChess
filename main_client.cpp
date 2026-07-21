#include <chrono>
#include <cstdlib>
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
#include "input/lobby_mapper.hpp"
#include "input/network_command_sink.hpp"
#include "input/network_lobby_sink.hpp"
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
#include "view/lobby_renderer.hpp"
#include "view/lobby_snapshot.hpp"
#include "view/panel_layout.hpp"
#include "view/renderer.hpp"
#include "view/sprite_library.hpp"
#include "view/window.hpp"

namespace {

constexpr char kServerUri[] = "ws://127.0.0.1:9002";
constexpr char kBoardImagePath[] = "images/board.png";
constexpr char kPiecesRoot[] = "images/pieces3";
constexpr char kWindowTitle[] = "KungFuChess (client)";
constexpr int kBoardDisplaySize = 435;
constexpr int kFrameDelayMs = 30;
constexpr int kAuthPollMs = 20;
constexpr int kQuitKey = 27;  // Esc
constexpr char kWaitingForOpponentStatus[] = "Waiting for an opponent...";
constexpr char kNoOpponentStatus[] = "No opponent found. Try again.";
constexpr char kRoomsNotAvailableStatus[] = "Rooms are coming soon.";

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

struct Credentials {
    std::string username;
    std::string password;
};

std::string askNonEmptyAscii(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string value;
        std::getline(std::cin, value);
        if (!value.empty() && isPlainAscii(value)) return value;
        std::cout << "Please use plain English letters and digits.\n";
    }
}

Credentials askForCredentials() {
    std::string username = askNonEmptyAscii("Username (English letters/digits only): ");
    std::cout << "Password: ";
    std::string password;
    std::getline(std::cin, password);
    return {username, password};
}

bool askWantsToRegister() {
    while (true) {
        std::cout << "Register a new account or log in to an existing one? (r/l): ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "r" || choice == "R") return true;
        if (choice == "l" || choice == "L") return false;
    }
}

// Filled in from the network thread's onMessage/onClose handlers as the
// server answers a register or login attempt, and read by the main thread's
// wait loops below. Reset between attempts so a retry never sees a stale
// answer from a previous one.
struct AuthOutcome {
    std::mutex mutex;
    std::optional<int> rating;
    std::optional<std::string> authRejection;
    bool registered = false;
    bool closed = false;
};

void resetAuthOutcome(AuthOutcome& outcome) {
    std::lock_guard<std::mutex> lock{outcome.mutex};
    outcome.rating.reset();
    outcome.authRejection.reset();
    outcome.registered = false;
}

// Blocks until the server confirms registration, rejects it (username taken),
// or the connection drops. A dropped connection has nothing left to retry
// against, so it ends the process; a rejection returns false so the caller
// can ask for different credentials.
bool awaitRegistration(AuthOutcome& outcome) {
    while (true) {
        {
            std::lock_guard<std::mutex> lock{outcome.mutex};
            if (outcome.registered) return true;
            if (outcome.authRejection) {
                std::cout << "Registration rejected: " << *outcome.authRejection << "\n";
                return false;
            }
            if (outcome.closed) {
                std::cerr << "Connection closed before registering\n";
                std::exit(1);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kAuthPollMs));
    }
}

// Blocks until the server confirms login (returning the account's rating),
// rejects it (bad credentials), or the connection drops. A bad login returns
// nullopt so the caller can ask again; a dropped connection has nothing left
// to retry against, so that ends the process instead.
std::optional<int> awaitLogin(AuthOutcome& outcome) {
    while (true) {
        {
            std::lock_guard<std::mutex> lock{outcome.mutex};
            if (outcome.rating) return outcome.rating;
            if (outcome.authRejection) {
                std::cout << "Login rejected: " << *outcome.authRejection << "\n";
                return std::nullopt;
            }
            if (outcome.closed) {
                std::cerr << "Connection closed before logging in\n";
                std::exit(1);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kAuthPollMs));
    }
}

// Drives the terminal register/login prompts until the server confirms a
// login. Registering succeeds into an automatic login attempt with the same
// credentials; either one being rejected loops back to ask again.
void askForCredentialsUntilLoggedIn(kfc::net::WebsocketppClient& transport,
                                    kfc::common::Logger& log, AuthOutcome& outcome) {
    while (true) {
        bool wantsRegister = askWantsToRegister();
        Credentials creds = askForCredentials();

        if (wantsRegister) {
            resetAuthOutcome(outcome);
            log.info("registering as " + creds.username);
            transport.send(kfc::protocol::encode(kfc::protocol::Message{
                kfc::protocol::RegisterRequest{creds.username, creds.password}}));
            if (!awaitRegistration(outcome)) continue;
            std::cout << "Registered. Logging in...\n";
        }

        resetAuthOutcome(outcome);
        log.info("logging in as " + creds.username);
        transport.send(kfc::protocol::encode(
            kfc::protocol::Message{kfc::protocol::LoginRequest{creds.username, creds.password}}));
        if (awaitLogin(outcome)) return;
    }
}

// Filled in from the network thread as the server answers a PLAY request, and
// read by the main thread each frame while it shows the lobby.
struct MatchOutcome {
    std::mutex mutex;
    std::optional<kfc::protocol::Matched> matched;
    bool noOpponent = false;
};

}  // namespace

// Composition root of the networked client. Logs in with an account in the
// terminal (registering first if needed), then shows a lobby screen with a
// PLAY button: clicking it queues for a match, and the game screen (unchanged
// from the pre-matchmaking client) only opens once the server reports a
// Matched opponent. No local engine anywhere: every click becomes a
// MoveIntent/JumpIntent, every frame drawn comes from the server's own
// broadcast snapshot. The transport's io loop runs on a background thread;
// the main thread owns OpenCV and the shared state it reads is guarded by a
// mutex throughout.
int main() {
    kfc::common::Logger log{"CLIENT"};

    AuthOutcome outcome;
    MatchOutcome matchOutcome;
    std::mutex stateMutex;
    std::optional<kfc::product::GameStateView> latest;

    kfc::net::WebsocketppClient transport;
    kfc::input::NetworkCommandSink commands{transport};
    kfc::input::NetworkLobbySink lobbySink{transport};

    transport.onOpen([&log]() { log.info("connected"); });
    transport.onClose([&log, &outcome]() {
        log.info("closed");
        std::lock_guard<std::mutex> lock{outcome.mutex};
        outcome.closed = true;
    });
    transport.onMessage([&outcome, &matchOutcome, &stateMutex, &latest](const std::string& text) {
        if (std::optional<kfc::protocol::Message> message = kfc::protocol::decode(text)) {
            if (const auto* loggedIn = std::get_if<kfc::protocol::LoggedIn>(&*message)) {
                std::lock_guard<std::mutex> lock{outcome.mutex};
                outcome.rating = loggedIn->rating;
            } else if (std::get_if<kfc::protocol::Registered>(&*message)) {
                std::lock_guard<std::mutex> lock{outcome.mutex};
                outcome.registered = true;
            } else if (const auto* authRejected =
                           std::get_if<kfc::protocol::AuthRejected>(&*message)) {
                std::lock_guard<std::mutex> lock{outcome.mutex};
                outcome.authRejection = authRejected->reason;
            } else if (const auto* matched = std::get_if<kfc::protocol::Matched>(&*message)) {
                std::lock_guard<std::mutex> lock{matchOutcome.mutex};
                matchOutcome.matched = *matched;
            } else if (std::get_if<kfc::protocol::NoOpponent>(&*message)) {
                std::lock_guard<std::mutex> lock{matchOutcome.mutex};
                matchOutcome.noOpponent = true;
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

    askForCredentialsUntilLoggedIn(transport, log, outcome);

    Img boardImage;
    boardImage.read(kBoardImagePath, {kBoardDisplaySize, kBoardDisplaySize},
                    /*keep_aspect=*/true);
    kfc::view::PanelLayout layout{boardImage.get_mat().cols,
                                  boardImage.get_mat().rows};
    kfc::view::Window window{kWindowTitle};
    kfc::view::LobbyRenderer lobbyRenderer{layout.canvasWidth(), layout.canvasHeight()};

    enum class Screen { kLobby, kGame };
    Screen screen = Screen::kLobby;
    std::optional<std::string> lobbyStatus;
    kfc::model::Color myColor = kfc::model::Color::kWhite;

    // Built lazily once the board's dimensions are known from the first
    // snapshot after a match is found.
    std::optional<kfc::view::SpriteLibrary> sprites;
    std::optional<kfc::view::Renderer> renderer;
    std::optional<kfc::model::Board> board;
    std::optional<kfc::input::Controller> controller;

    Clock::time_point start = Clock::now();
    while (true) {
        if (screen == Screen::kLobby) {
            std::lock_guard<std::mutex> lock{matchOutcome.mutex};
            if (matchOutcome.matched) {
                myColor = matchOutcome.matched->color;
                screen = Screen::kGame;
                lobbyStatus.reset();
            } else if (matchOutcome.noOpponent) {
                lobbyStatus = kNoOpponentStatus;
                matchOutcome.noOpponent = false;
            }
        }

        if (screen == Screen::kLobby) {
            for (const kfc::view::MouseEvent& event : window.takeMouseEvents()) {
                if (event.type != kfc::view::MouseEvent::Type::kClick) continue;
                switch (kfc::input::hitTest(lobbyRenderer.layout(), event.x, event.y)) {
                    case kfc::input::LobbyAction::kPlay:
                        lobbySink.requestPlay();
                        lobbyStatus = kWaitingForOpponentStatus;
                        break;
                    case kfc::input::LobbyAction::kEnterRoom:
                        lobbyStatus = kRoomsNotAvailableStatus;
                        break;
                    case kfc::input::LobbyAction::kNone:
                        break;
                }
            }
            window.show(lobbyRenderer.render(kfc::view::LobbyFrame{lobbyStatus}));
        } else {
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
        }

        if (window.waitKey(kFrameDelayMs) == kQuitKey) break;
    }

    transport.stop();
    network.join();
    return 0;
}
