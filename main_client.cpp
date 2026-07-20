#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "common/logger.hpp"
#include "img.hpp"
#include "net/websocketpp_transport.hpp"
#include "product/game_state_view.hpp"
#include "protocol/wire_snapshot.hpp"
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
constexpr int kBoardDisplaySize = 640;
constexpr int kFrameDelayMs = 30;
constexpr int kQuitKey = 27;  // Esc

using Clock = std::chrono::steady_clock;

int elapsedMsSince(Clock::time_point start) {
    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                              start)
            .count());
}

}  // namespace

// Step 8b client: a display-only window over the network. It holds no engine;
// it renders whatever authoritative snapshot the server broadcasts. The
// transport's io loop runs on a background thread and only stores the latest
// decoded snapshot under a mutex; the main thread owns OpenCV and reads that
// snapshot to draw. There is no input yet — that is step 9.
int main() {
    kfc::common::Logger log{"CLIENT"};

    std::mutex mutex;
    std::optional<kfc::product::GameStateView> latest;

    kfc::net::WebsocketppClient transport;
    transport.onOpen([&log]() { log.info("connected"); });
    transport.onClose([&log]() { log.info("closed"); });
    transport.onMessage([&mutex, &latest](const std::string& text) {
        std::optional<kfc::product::GameStateView> decoded =
            kfc::protocol::decodeSnapshot(text);
        if (!decoded) return;
        std::lock_guard<std::mutex> lock{mutex};
        latest = std::move(decoded);
    });

    log.info(std::string{"connecting to "} + kServerUri);
    transport.connect(kServerUri);
    std::thread network([&transport] { transport.run(); });

    Img boardImage;
    boardImage.read(kBoardImagePath, {kBoardDisplaySize, kBoardDisplaySize},
                    /*keep_aspect=*/true);
    kfc::view::PanelLayout layout{boardImage.get_mat().cols,
                                  boardImage.get_mat().rows};
    kfc::view::Window window{kWindowTitle};

    // The renderer needs the board's dimensions, which arrive with the first
    // snapshot, so it is built once that snapshot is in hand. The sprite library
    // must outlive it, so both are held here as the renderer stores a reference.
    std::optional<kfc::view::SpriteLibrary> sprites;
    std::optional<kfc::view::Renderer> renderer;

    Clock::time_point start = Clock::now();
    while (true) {
        std::optional<kfc::product::GameStateView> current;
        {
            std::lock_guard<std::mutex> lock{mutex};
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
            }
            window.show(renderer->render(
                kfc::view::buildSnapshot(std::move(*current), std::nullopt, {}),
                elapsedMsSince(start)));
        }

        if (window.waitKey(kFrameDelayMs) == kQuitKey) break;
    }

    transport.stop();
    network.join();
    return 0;
}
