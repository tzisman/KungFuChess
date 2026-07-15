#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "engine/game_engine.hpp"
#include "img.hpp"
#include "input/board_mapper.hpp"
#include "input/controller.hpp"
#include "io/board_parser.hpp"
#include "io/piece_codec.hpp"
#include "io/piece_config.hpp"
#include "model/board.hpp"
#include "model/piece.hpp"
#include "realtime/motion.hpp"
#include "view/board_geometry.hpp"
#include "view/game_snapshot.hpp"
#include "view/renderer.hpp"
#include "view/sprite_library.hpp"
#include "view/window.hpp"

namespace {

constexpr char kBoardImagePath[] = "images/board.png";
constexpr char kPiecesRoot[] = "images/pieces2";
constexpr char kStartBoardPath[] = "boards/start.txt";
constexpr char kWindowTitle[] = "KungFuChess";

// How large the board is drawn on screen. The window sizes itself to the
// picture, so this is the one number to change to get a bigger or smaller
// board; everything else is measured from it.
constexpr int kBoardDisplaySize = 640;

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

}  // namespace

int main() {
    try {
        std::ifstream startBoard{kStartBoardPath};
        if (!startBoard) {
            std::cerr << "Cannot open " << kStartBoardPath << "\n";
            return 1;
        }

        kfc::io::ParsedInput parsed = kfc::io::parseInput(startBoard);
        kfc::engine::GameEngine engine{std::move(parsed.board),
                                       loadMotionProfiles(kPiecesRoot)};
        const kfc::model::Board& board = engine.board();

        // The geometry is measured from the board image scaled to the display
        // size, keeping its aspect ratio, so a different board picture or a
        // different size needs no code change anywhere else.
        Img boardImage;
        boardImage.read(kBoardImagePath, {kBoardDisplaySize, kBoardDisplaySize},
                        /*keep_aspect=*/true);
        kfc::view::BoardGeometry geometry{boardImage.get_mat().cols,
                                          boardImage.get_mat().rows,
                                          board.width(), board.height()};

        kfc::view::SpriteLibrary sprites{kPiecesRoot, geometry.cellWidth(),
                                         geometry.cellHeight()};
        kfc::view::Renderer renderer{kBoardImagePath, sprites, geometry};
        kfc::view::Window window{kWindowTitle};

        kfc::input::BoardMapper mapper{geometry};
        kfc::input::Controller controller{engine, mapper};

        Clock::time_point start = Clock::now();
        Clock::time_point last = start;
        while (true) {
            for (const kfc::view::MouseEvent& event : window.takeMouseEvents()) {
                dispatch(event, controller);
            }

            engine.advance(elapsedMsSince(last));

            int nowMs = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(last -
                                                                      start)
                    .count());
            window.show(renderer.render(
                kfc::view::buildSnapshot(engine, controller.selection()),
                nowMs));

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
