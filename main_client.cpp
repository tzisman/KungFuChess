#include <chrono>
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
// Terminal registration (asking for a real name and waiting for Assigned
// before opening the window) is step 11; this placeholder is enough for the
// server to have a name and hand back a colour.
constexpr char kPlayerName[] = "Player";
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

}  // namespace

// Step 10 client: a full networked player. It holds no engine; every click
// becomes a MoveIntent/JumpIntent sent to the authoritative server through
// NetworkCommandSink, and every frame drawn comes from the server's own
// broadcast snapshot. The transport's io loop runs on a background thread and
// only stores the latest decoded message under a mutex; the main thread owns
// OpenCV and reads that state to draw and to drive the shared Controller.
int main() {
    kfc::common::Logger log{"CLIENT"};

    std::mutex mutex;
    std::optional<kfc::product::GameStateView> latest;
    std::optional<kfc::model::Color> myColor;

    kfc::net::WebsocketppClient transport;
    kfc::input::NetworkCommandSink commands{transport};

    transport.onOpen([&log, &transport]() {
        log.info("connected -> joining");
        transport.send(kfc::protocol::encode(
            kfc::protocol::Message{kfc::protocol::JoinRequest{kPlayerName}}));
    });
    transport.onClose([&log]() { log.info("closed"); });
    transport.onMessage([&mutex, &latest, &myColor](const std::string& text) {
        if (std::optional<kfc::protocol::Message> message =
                kfc::protocol::decode(text)) {
            if (const auto* assigned =
                    std::get_if<kfc::protocol::Assigned>(&*message)) {
                std::lock_guard<std::mutex> lock{mutex};
                myColor = assigned->color;
            }
            return;
        }
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

    // Built lazily: the renderer once the board's dimensions are known from
    // the first snapshot, and the controller once the assigned colour is also
    // known (the two can arrive in either order over the wire).
    std::optional<kfc::view::SpriteLibrary> sprites;
    std::optional<kfc::view::Renderer> renderer;
    std::optional<kfc::model::Board> board;
    std::optional<kfc::input::Controller> controller;

    Clock::time_point start = Clock::now();
    while (true) {
        std::optional<kfc::product::GameStateView> current;
        std::optional<kfc::model::Color> color;
        {
            std::lock_guard<std::mutex> lock{mutex};
            current = latest;
            color = myColor;
        }

        if (current) {
            kfc::view::BoardGeometry geometry{
                boardImage.get_mat().cols, boardImage.get_mat().rows,
                current->boardWidth, current->boardHeight,
                layout.boardOrigin()};

            if (!renderer) {
                sprites.emplace(kPiecesRoot, geometry.cellWidth(),
                                geometry.cellHeight());
                renderer.emplace(kBoardImagePath, *sprites, geometry, layout);
            }
            if (!controller && color) {
                board.emplace(current->boardWidth, current->boardHeight);
                controller.emplace(
                    kfc::app::makeController(*board, commands, geometry, color));
            }
            if (board) syncBoard(*board, current->pieces);

            if (controller) {
                for (const kfc::view::MouseEvent& event :
                    window.takeMouseEvents()) {
                    dispatch(event, *controller);
                }
            }

            std::optional<kfc::model::Position> selection =
                controller ? controller->selection() : std::nullopt;
            window.show(renderer->render(
                kfc::view::buildSnapshot(std::move(*current), selection, {}),
                elapsedMsSince(start)));
        }

        if (window.waitKey(kFrameDelayMs) == kQuitKey) break;
    }

    transport.stop();
    network.join();
    return 0;
}
