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
#include "input/controller.hpp"
#include "input/lobby_mapper.hpp"
#include "input/network_command_sink.hpp"
#include "input/network_lobby_sink.hpp"
#include "input/text_prompt_buffer.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "net/websocketpp_transport.hpp"
#include "product/game_state_view.hpp"
#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"
#include "protocol/wire_snapshot.hpp"
#include "rules/rule_engine.hpp"
#include "view/countdown_overlay.hpp"
#include "view/game_snapshot.hpp"
#include "view/lobby_renderer.hpp"
#include "view/lobby_snapshot.hpp"
#include "view/resize_watcher.hpp"
#include "view/text_prompt.hpp"
#include "view/window.hpp"

namespace {

constexpr char kServerUri[] = "ws://127.0.0.1:9002";
constexpr char kBoardImagePath[] = "images/board.png";
constexpr char kPiecesRoot[] = "images/pieces3";
constexpr char kWindowTitle[] = "KungFuChess (client)";
constexpr int kBoardDisplaySize = 435;
constexpr int kFrameDelayMs = 30;
constexpr int kAuthPollMs = 20;
// The server broadcasts a CountdownTick once a second while a disconnect
// countdown runs; if none has arrived for longer than that, the countdown is
// over (reconnect or forfeit) and the overlay should stop showing it.
constexpr int kCountdownStaleMs = 1500;
constexpr int kQuitKey = 27;  // Esc
constexpr char kWaitingForOpponentStatus[] = "Waiting for an opponent...";
constexpr char kNoOpponentStatus[] = "No opponent found. Try again.";
constexpr char kEnterRoomPromptLabel[] = "Enter room name, then press Enter:";
constexpr char kJoiningRoomStatus[] = "Joining room...";

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

// Checks whether the window has settled at a new size and, if so, rebuilds
// everything measured from it: the shared presentation (so the lobby and
// room-prompt screens follow immediately), and, once a match is underway,
// the game view and the controller's click mapping. Finishes by snapping the
// window to the exact natural size for the new height, so what's on screen
// is never a stretched or letterboxed copy of what was actually rendered.
void handleResize(kfc::view::Window& window, kfc::view::ResizeWatcher& watcher,
                  int elapsedMs, const std::string& boardImagePath,
                  const std::string& piecesRoot,
                  kfc::app::Presentation& presentation,
                  kfc::view::LobbyRenderer& lobbyRenderer,
                  kfc::view::TextPromptRenderer& roomPromptRenderer,
                  std::optional<kfc::app::GameView>& gameView,
                  std::optional<kfc::input::Controller>& controller) {
    std::optional<kfc::view::WindowSize> settled =
        watcher.poll(window.contentSize(), elapsedMs);
    if (!settled) return;

    int boardCols = gameView ? gameView->geometry.cols() : 0;
    int boardRows = gameView ? gameView->geometry.rows() : 0;

    presentation = kfc::app::buildPresentation(boardImagePath, settled->height);
    lobbyRenderer = kfc::view::LobbyRenderer{presentation.layout.canvasWidth(),
                                             presentation.layout.canvasHeight()};
    roomPromptRenderer = kfc::view::TextPromptRenderer{
        presentation.layout.canvasWidth(), presentation.layout.canvasHeight()};

    if (gameView) {
        gameView.emplace(boardImagePath, piecesRoot, presentation, boardCols,
                         boardRows);
        controller->setGeometry(gameView->geometry);
    }

    kfc::view::WindowSize natural{presentation.layout.canvasWidth(),
                                  presentation.layout.canvasHeight()};
    window.resizeTo(natural);
    watcher.reset(natural);
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

std::string roomRoleName(kfc::protocol::Role role) {
    switch (role) {
        case kfc::protocol::Role::kWhite: return "White";
        case kfc::protocol::Role::kBlack: return "Black";
        case kfc::protocol::Role::kSpectator: return "Spectator";
    }
    return "Spectator";
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
bool awaitRegistration(AuthOutcome& outcome, kfc::common::Logger& log) {
    while (true) {
        {
            std::lock_guard<std::mutex> lock{outcome.mutex};
            if (outcome.registered) {
                log.info("registration succeeded");
                return true;
            }
            if (outcome.authRejection) {
                std::cout << "Registration rejected: " << *outcome.authRejection << "\n";
                log.info("registration rejected: " + *outcome.authRejection);
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
std::optional<int> awaitLogin(AuthOutcome& outcome, kfc::common::Logger& log) {
    while (true) {
        {
            std::lock_guard<std::mutex> lock{outcome.mutex};
            if (outcome.rating) {
                log.info("login succeeded, rating " + std::to_string(*outcome.rating));
                return outcome.rating;
            }
            if (outcome.authRejection) {
                std::cout << "Login rejected: " << *outcome.authRejection << "\n";
                log.info("login rejected: " + *outcome.authRejection);
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
            if (!awaitRegistration(outcome, log)) continue;
            std::cout << "Registered. Logging in...\n";
        }

        resetAuthOutcome(outcome);
        log.info("logging in as " + creds.username);
        transport.send(kfc::protocol::encode(
            kfc::protocol::Message{kfc::protocol::LoginRequest{creds.username, creds.password}}));
        if (awaitLogin(outcome, log)) return;
    }
}

// Filled in from the network thread as the server answers a PLAY request, and
// read by the main thread each frame while it shows the lobby.
struct MatchOutcome {
    std::mutex mutex;
    std::optional<kfc::protocol::Matched> matched;
    bool noOpponent = false;
};

// Filled in from the network thread as the server answers an EnterRoomRequest,
// and read by the main thread each frame while it shows the room-name prompt.
struct RoomOutcome {
    std::mutex mutex;
    std::optional<kfc::protocol::RoomJoined> joined;
};

// Filled in from the network thread each time a CountdownTick arrives, and
// read by the main thread every frame while it shows the game screen.
// lastUpdate lets the main thread notice staleness itself (see
// kCountdownStaleMs) — the server never sends an explicit "cancelled"
// message for a countdown a reconnect called off.
struct CountdownState {
    std::mutex mutex;
    std::optional<int> secondsLeft;
    Clock::time_point lastUpdate;
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
    RoomOutcome roomOutcome;
    CountdownState countdown;
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
    transport.onMessage([&log, &outcome, &matchOutcome, &roomOutcome, &countdown, &stateMutex,
                         &latest](const std::string& text) {
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
                log.info(std::string{"matched as "} + kfc::model::nameOf(matched->color) +
                         " against " + matched->opponentName);
                std::lock_guard<std::mutex> lock{matchOutcome.mutex};
                matchOutcome.matched = *matched;
            } else if (std::get_if<kfc::protocol::NoOpponent>(&*message)) {
                log.info("no opponent found");
                std::lock_guard<std::mutex> lock{matchOutcome.mutex};
                matchOutcome.noOpponent = true;
            } else if (const auto* roomJoined = std::get_if<kfc::protocol::RoomJoined>(&*message)) {
                log.info("room joined as " + roomRoleName(roomJoined->role));
                std::lock_guard<std::mutex> lock{roomOutcome.mutex};
                roomOutcome.joined = *roomJoined;
            } else if (const auto* tick = std::get_if<kfc::protocol::CountdownTick>(&*message)) {
                std::lock_guard<std::mutex> lock{countdown.mutex};
                if (!countdown.secondsLeft) log.info("disconnect countdown observed");
                countdown.secondsLeft = tick->secondsLeft;
                countdown.lastUpdate = Clock::now();
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
    std::optional<int> myRating;
    {
        std::lock_guard<std::mutex> lock{outcome.mutex};
        myRating = outcome.rating;
    }

    kfc::app::Presentation presentation =
        kfc::app::buildPresentation(kBoardImagePath, kBoardDisplaySize);
    kfc::view::Window window{kWindowTitle};
    kfc::view::LobbyRenderer lobbyRenderer{presentation.layout.canvasWidth(),
                                           presentation.layout.canvasHeight()};
    kfc::view::TextPromptRenderer roomPromptRenderer{
        presentation.layout.canvasWidth(), presentation.layout.canvasHeight()};
    kfc::view::ResizeWatcher resizeWatcher{
        {presentation.layout.canvasWidth(), presentation.layout.canvasHeight()}};

    enum class Screen { kLobby, kEnterRoom, kGame };
    Screen screen = Screen::kLobby;
    std::optional<std::string> lobbyStatus;
    std::optional<kfc::model::Color> myColor;
    kfc::input::TextPromptBuffer roomNameBuffer;
    bool roomRequestSent = false;

    // Built lazily once the board's dimensions are known from the first
    // snapshot after a match is found.
    std::optional<kfc::app::GameView> gameView;
    std::optional<kfc::model::Board> board;
    std::optional<kfc::input::Controller> controller;
    bool gameOverLogged = false;

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

        if (screen == Screen::kEnterRoom) {
            std::lock_guard<std::mutex> lock{roomOutcome.mutex};
            if (roomOutcome.joined) {
                myColor = roomOutcome.joined->color;
                screen = Screen::kGame;
                roomOutcome.joined.reset();
            }
        }

        if (screen == Screen::kLobby) {
            for (const kfc::view::MouseEvent& event : window.takeMouseEvents()) {
                if (event.type != kfc::view::MouseEvent::Type::kClick) continue;
                switch (kfc::input::hitTest(lobbyRenderer.layout(), event.x, event.y)) {
                    case kfc::input::LobbyAction::kPlay:
                        log.info("PLAY pressed");
                        lobbySink.requestPlay();
                        lobbyStatus = kWaitingForOpponentStatus;
                        break;
                    case kfc::input::LobbyAction::kEnterRoom:
                        screen = Screen::kEnterRoom;
                        roomNameBuffer.clear();
                        roomRequestSent = false;
                        break;
                    case kfc::input::LobbyAction::kNone:
                        break;
                }
            }
            window.show(lobbyRenderer.render(kfc::view::LobbyFrame{myRating, lobbyStatus}));
        } else if (screen == Screen::kEnterRoom) {
            window.show(roomPromptRenderer.render(
                roomRequestSent ? kJoiningRoomStatus : kEnterRoomPromptLabel,
                roomNameBuffer.text()));
        } else {
            std::optional<kfc::product::GameStateView> current;
            {
                std::lock_guard<std::mutex> lock{stateMutex};
                current = latest;
            }

            if (current) {
                if (!gameView) {
                    gameView.emplace(kBoardImagePath, kPiecesRoot, presentation,
                                     current->boardWidth, current->boardHeight);
                    board.emplace(current->boardWidth, current->boardHeight);
                    controller.emplace(kfc::app::makeController(
                        *board, commands, gameView->geometry, myColor,
                        /*interactive=*/myColor.has_value()));
                }
                syncBoard(*board, current->pieces);
                if (current->gameOver && !gameOverLogged) {
                    log.info("game over");
                    gameOverLogged = true;
                }

                for (const kfc::view::MouseEvent& event : window.takeMouseEvents()) {
                    dispatch(event, *controller);
                }

                std::vector<kfc::model::Position> moveTargets;
                if (std::optional<kfc::model::Position> selection = controller->selection()) {
                    moveTargets =
                        legalDestinationsFrom(*board, *selection, current->gameOver);
                }

                std::optional<std::string> overlayText;
                {
                    std::lock_guard<std::mutex> lock{countdown.mutex};
                    if (countdown.secondsLeft &&
                        elapsedMsSince(countdown.lastUpdate) < kCountdownStaleMs) {
                        overlayText = kfc::view::countdownText(*countdown.secondsLeft);
                    }
                }

                window.show(gameView->renderer.render(
                    kfc::view::buildSnapshot(std::move(*current),
                                             controller->selection(),
                                             std::move(moveTargets)),
                    elapsedMsSince(start), overlayText));
            }
        }

        // Checked only after a frame has actually been shown: a WINDOW_NORMAL
        // window adopts the size of the first image shown into it, so this
        // is the first point at which its content size reliably reflects
        // something real rather than a toolkit default.
        handleResize(window, resizeWatcher, kFrameDelayMs, kBoardImagePath,
                    kPiecesRoot, presentation, lobbyRenderer,
                    roomPromptRenderer, gameView, controller);

        int key = window.waitKey(kFrameDelayMs);
        if (screen == Screen::kEnterRoom && !roomRequestSent) {
            if (roomNameBuffer.handleKey(key) && !roomNameBuffer.text().empty()) {
                log.info("entering room '" + roomNameBuffer.text() + "'");
                lobbySink.requestEnterRoom(roomNameBuffer.text());
                roomRequestSent = true;
            }
        }
        if (key == kQuitKey) break;
    }

    transport.stop();
    network.join();
    return 0;
}
