#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "app/composition.hpp"
#include "engine/game_engine.hpp"
#include "input/controller.hpp"
#include "input/engine_command_sink.hpp"
#include "io/board_parser.hpp"
#include "io/piece_codec.hpp"
#include "io/piece_config.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "product/move_log.hpp"
#include "product/score_board.hpp"
#include "realtime/motion.hpp"
#include "view/game_snapshot.hpp"
#include "view/resize_watcher.hpp"
#include "view/window.hpp"

namespace {

constexpr char kBoardImagePath[] = "images/board.png";
constexpr char kPiecesRoot[] = "images/pieces3";
constexpr char kStartBoardPath[] = "texttests/start.txt";
constexpr char kWindowTitle[] = "KungFuChess";

// How large the board is drawn on screen at startup; everything else is
// measured from it. The window is user-resizable from there on — see
// handleResize below.
constexpr int kBoardDisplaySize = 435;

constexpr int kFrameDelayMs = 30;
constexpr int kQuitKey = 27;  // Esc

using Clock = std::chrono::steady_clock;

int msPerSquare(double squaresPerSec) {
    return static_cast<int>(std::lround(1000.0 / squaresPerSec));
}

double speedInConfig(const std::string& piecesRoot, kfc::model::PieceKind kind,
                     const char* stateDir) {
    std::string path = piecesRoot;
    path += '/';
    path += kfc::io::kindLetter(kind);
    path += 'W';
    path += "/states/";
    path += stateDir;
    path += "/config.json";

    std::ifstream file{path};
    if (!file) throw std::runtime_error("Cannot open " + path);
    return kfc::io::parseStateConfig(file).speedSquaresPerSec;
}

// The physics side of the piece configs, turned into the timings the logic
// runs on. Both colours share one config, so the white pieces' files speak
// for both.
kfc::realtime::MotionProfiles loadMotionProfiles(const std::string& piecesRoot) {
    kfc::realtime::MotionProfiles profiles;
    for (kfc::model::PieceKind kind : kfc::model::kAllPieceKinds) {
        profiles.setTiming(
            kind, msPerSquare(speedInConfig(piecesRoot, kind, "move")),
            kfc::realtime::kJumpDurationFactor *
                msPerSquare(speedInConfig(piecesRoot, kind, "jump")));
    }
    return profiles;
}

void dispatch(const kfc::view::MouseEvent& event,
              kfc::input::Controller& controller) {
    if (event.type == kfc::view::MouseEvent::Type::kDoubleClick) {
        controller.handleJump(event.x, event.y);
    } else {
        controller.handleClick(event.x, event.y);
    }
}

int elapsedMsSince(Clock::time_point& last) {
    Clock::time_point now = Clock::now();
    int elapsed = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last)
            .count());
    last = now;
    return elapsed;
}

// Checks whether the window has settled at a new size and, if so, rebuilds
// the game view in place at that size and re-points the controller's click
// mapping to match. Finishes by snapping the window to the exact natural
// size for the new height, so what's on screen is never a stretched or
// letterboxed copy of what was actually rendered — then re-syncs the watcher
// from the size the window actually reports afterward, not the size that was
// requested, since the two can differ (DPI scaling, border/title-bar insets)
// and seeding from the request instead of reality reads as a fresh resize
// every cycle, drifting the window further on every frame.
void handleResize(kfc::view::Window& window, kfc::view::ResizeWatcher& watcher,
                  int elapsedMs, const std::string& boardImagePath,
                  const std::string& piecesRoot,
                  const kfc::model::Board& board,
                  kfc::app::Presentation& presentation,
                  std::optional<kfc::app::GameView>& gameView,
                  kfc::input::Controller& controller) {
    std::optional<kfc::view::WindowSize> settled =
        watcher.poll(window.contentSize(), elapsedMs);
    if (!settled) return;

    presentation = kfc::app::buildPresentation(boardImagePath, settled->height);
    gameView.emplace(boardImagePath, piecesRoot, presentation, board.width(),
                     board.height());
    controller.setGeometry(gameView->geometry);

    kfc::view::WindowSize natural{presentation.layout.canvasWidth(),
                                  presentation.layout.canvasHeight()};
    window.resizeTo(natural);
    watcher.reset(window.contentSize());
}

}  // namespace

int main() {
    try {
        std::ifstream startBoard{kStartBoardPath};
        if (!startBoard) {
            std::cerr << "Cannot open " << kStartBoardPath << "\n";
            return 1;
        }

        kfc::io::ParsedInput parsed = kfc::io::parseInput(startBoard);

        // Declared before the engine so they outlive it.
        kfc::product::ScoreBoard scores;
        kfc::product::MoveLog moveLog;

        kfc::engine::GameEngine engine{std::move(parsed.board),
                                       loadMotionProfiles(kPiecesRoot)};
        scores.subscribeTo(engine.events());
        moveLog.subscribeTo(engine.events());
        const kfc::model::Board& board = engine.board();

        kfc::app::Presentation presentation =
            kfc::app::buildPresentation(kBoardImagePath, kBoardDisplaySize);
        std::optional<kfc::app::GameView> gameView;
        gameView.emplace(kBoardImagePath, kPiecesRoot, presentation,
                         board.width(), board.height());
        kfc::view::Window window{kWindowTitle};
        kfc::view::ResizeWatcher resizeWatcher{
            {presentation.layout.canvasWidth(),
             presentation.layout.canvasHeight()}};

        kfc::input::EngineCommandSink commands{engine};
        kfc::input::Controller controller =
            kfc::app::makeController(board, commands, gameView->geometry);

        Clock::time_point start = Clock::now();
        Clock::time_point last = start;
        while (true) {
            for (const kfc::view::MouseEvent& event : window.takeMouseEvents()) {
                dispatch(event, controller);
            }

            int elapsed = elapsedMsSince(last);
            engine.advance(elapsed);

            int nowMs = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(last -
                                                                      start)
                    .count());
            window.show(gameView->renderer.render(
                kfc::view::buildSnapshot(engine, controller.selection(), scores,
                                         moveLog),
                nowMs));

            // Checked only after a frame has actually been shown: a
            // WINDOW_NORMAL window adopts the size of the first image shown
            // into it, so this is the first point at which its content size
            // reliably reflects something real rather than a toolkit default.
            handleResize(window, resizeWatcher, elapsed, kBoardImagePath,
                        kPiecesRoot, board, presentation, gameView, controller);

            if (window.waitKey(kFrameDelayMs) == kQuitKey) break;
        }
    } catch (const kfc::io::ParseError& error) {
        std::cerr << "Parse error: " << error.code << "\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
