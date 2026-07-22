#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "app/composition.hpp"
#include "app/controller_factory.hpp"
#include "app/network_session.hpp"
#include "common/logger.hpp"
#include "input/command_sink.hpp"
#include "input/controller.hpp"
#include "input/network_lobby_sink.hpp"
#include "input/text_prompt_buffer.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "view/screens/lobby_renderer.hpp"
#include "view/resize_watcher.hpp"
#include "view/screens/text_prompt.hpp"
#include "view/window.hpp"

namespace kfc::app {

// Drives the client's on-screen state machine once login has succeeded: a
// lobby screen (PLAY / enter-room), a room-name prompt, and the game screen,
// switching between them as the session reports matchmaking and room
// answers. No local engine anywhere: every click becomes a command sent
// through commands/lobbySink, and every frame drawn comes from the session's
// own broadcast snapshot.
class ClientApp {
public:
    ClientApp(NetworkSession& session, input::CommandSink& commands,
              input::NetworkLobbySink& lobbySink, common::Logger& log,
              std::optional<int> myRating);

    // Runs the screen loop until the user presses Escape.
    void run();

private:
    enum class Screen { kLobby, kEnterRoom, kGame };
    using Clock = std::chrono::steady_clock;

    void checkMatchmakingOutcome();
    void checkRoomOutcome();
    void handleLobbyClicks();
    void renderLobby();
    void renderEnterRoom();
    void renderGame();
    void handleResize();

    NetworkSession& session_;
    input::CommandSink& commands_;
    input::NetworkLobbySink& lobbySink_;
    common::Logger& log_;
    std::optional<int> myRating_;

    Presentation presentation_;
    view::Window window_;
    view::LobbyRenderer lobbyRenderer_;
    view::TextPromptRenderer roomPromptRenderer_;
    view::ResizeWatcher resizeWatcher_;

    Screen screen_ = Screen::kLobby;
    std::optional<std::string> lobbyStatus_;
    std::optional<model::Color> myColor_;
    input::TextPromptBuffer roomNameBuffer_;
    bool roomRequestSent_ = false;

    // Built lazily once the board's dimensions are known from the first
    // snapshot after a match is found.
    std::optional<GameView> gameView_;
    std::optional<model::Board> board_;
    std::optional<input::Controller> controller_;
    bool gameOverLogged_ = false;

    Clock::time_point start_ = Clock::now();
};

}
