#include "app/client_app.hpp"

#include <utility>
#include <vector>

#include "input/lobby_mapper.hpp"
#include "model/board.hpp"
#include "rules/rule_engine.hpp"
#include "view/countdown_overlay.hpp"
#include "view/game_snapshot.hpp"
#include "view/screens/lobby_renderer.hpp"

namespace kfc::app {
namespace {

constexpr char kBoardImagePath[] = "images/board.png";
constexpr char kPiecesRoot[] = "images/pieces3";
constexpr char kWindowTitle[] = "KungFuChess (client)";
// How large the board is drawn on startup; every other measurement follows
// from it. The window is user-resizable from there — see handleResize.
constexpr int kBoardDisplaySize = 700;
constexpr int kFrameDelayMs = 30;
constexpr int kQuitKey = 27;  // Esc
constexpr char kWaitingForOpponentStatus[] = "Waiting for an opponent...";
constexpr char kNoOpponentStatus[] = "No opponent found. Try again.";
constexpr char kEnterRoomPromptLabel[] = "Enter room name, then press Enter:";
constexpr char kJoiningRoomStatus[] = "Joining room...";

int elapsedMsSince(std::chrono::steady_clock::time_point start) {
    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
            .count());
}

void dispatchMouseEvent(const view::MouseEvent& event, input::Controller& controller) {
    if (event.type == view::MouseEvent::Type::kDoubleClick) {
        controller.handleJump(event.x, event.y);
    } else {
        controller.handleClick(event.x, event.y);
    }
}

void syncBoard(model::Board& board, const std::vector<product::PieceSnapshot>& pieces) {
    board.clear();
    model::PieceId id = 0;
    for (const product::PieceSnapshot& piece : pieces) {
        if (board.pieceAt(piece.cell)) continue;
        board.addPiece(model::Piece{id++, piece.color, piece.kind, piece.cell, piece.state});
    }
}

// The same legality the offline path gets from GameEngine::legalDestinationsFrom
// (rules::RuleEngine plus an idle, game-not-over guard), computed against the
// client's own local mirror instead of an engine it does not have. This is a
// display hint only: the server enforces the real rule when the move actually
// arrives, so a stale frame here costs nothing but a momentarily wrong dot.
std::vector<model::Position> legalDestinationsFrom(const model::Board& board,
                                                    model::Position from, bool gameOver) {
    if (gameOver) return {};
    std::optional<model::Piece> piece = board.pieceAt(from);
    if (!piece || piece->state() != model::PieceState::kIdle) return {};

    rules::RuleEngine ruleEngine;
    rules::Destinations destinations = ruleEngine.legalDestinations(board, from);
    return {destinations.begin(), destinations.end()};
}

}  // namespace

ClientApp::ClientApp(NetworkSession& session, input::CommandSink& commands,
                     input::NetworkLobbySink& lobbySink, common::Logger& log,
                     std::optional<int> myRating)
    : session_(session),
      commands_(commands),
      lobbySink_(lobbySink),
      log_(log),
      myRating_(myRating),
      presentation_(buildPresentation(kBoardImagePath, kBoardDisplaySize)),
      window_(kWindowTitle, {presentation_.layout.canvasWidth(),
                             presentation_.layout.canvasHeight()}),
      lobbyRenderer_(presentation_.layout.canvasWidth(), presentation_.layout.canvasHeight()),
      roomPromptRenderer_(presentation_.layout.canvasWidth(),
                          presentation_.layout.canvasHeight()),
      resizeWatcher_({presentation_.layout.canvasWidth(),
                      presentation_.layout.canvasHeight()}) {}

void ClientApp::run() {
    while (true) {
        if (screen_ == Screen::kLobby) checkMatchmakingOutcome();
        if (screen_ == Screen::kEnterRoom) checkRoomOutcome();

        if (screen_ == Screen::kLobby) {
            handleLobbyClicks();
            renderLobby();
        } else if (screen_ == Screen::kEnterRoom) {
            renderEnterRoom();
        } else {
            renderGame();
        }

        handleResize();

        int key = window_.waitKey(kFrameDelayMs);
        if (screen_ == Screen::kEnterRoom && !roomRequestSent_) {
            if (roomNameBuffer_.handleKey(key) && !roomNameBuffer_.text().empty()) {
                log_.info("entering room '" + roomNameBuffer_.text() + "'");
                lobbySink_.requestEnterRoom(roomNameBuffer_.text());
                roomRequestSent_ = true;
            }
        }
        if (key == kQuitKey) break;
    }
}

void ClientApp::checkMatchmakingOutcome() {
    if (std::optional<protocol::Matched> matched = session_.takeMatched()) {
        myColor_ = matched->color;
        screen_ = Screen::kGame;
        lobbyStatus_.reset();
    } else if (session_.takeNoOpponent()) {
        lobbyStatus_ = kNoOpponentStatus;
    }
}

void ClientApp::checkRoomOutcome() {
    if (std::optional<protocol::RoomJoined> joined = session_.takeRoomJoined()) {
        myColor_ = joined->color;
        screen_ = Screen::kGame;
    }
}

void ClientApp::handleLobbyClicks() {
    for (const view::MouseEvent& event : window_.takeMouseEvents()) {
        if (event.type != view::MouseEvent::Type::kClick) continue;
        switch (input::hitTest(lobbyRenderer_.layout(), event.x, event.y)) {
            case input::LobbyAction::kPlay:
                log_.info("PLAY pressed");
                lobbySink_.requestPlay();
                lobbyStatus_ = kWaitingForOpponentStatus;
                break;
            case input::LobbyAction::kEnterRoom:
                screen_ = Screen::kEnterRoom;
                roomNameBuffer_.clear();
                roomRequestSent_ = false;
                break;
            case input::LobbyAction::kNone:
                break;
        }
    }
}

void ClientApp::renderLobby() {
    window_.show(lobbyRenderer_.render(view::LobbyFrame{myRating_, lobbyStatus_}));
}

void ClientApp::renderEnterRoom() {
    window_.show(roomPromptRenderer_.render(
        roomRequestSent_ ? kJoiningRoomStatus : kEnterRoomPromptLabel, roomNameBuffer_.text()));
}

void ClientApp::renderGame() {
    std::optional<product::GameStateView> current = session_.latestSnapshot();
    if (!current) return;

    if (!gameView_) {
        gameView_.emplace(kBoardImagePath, kPiecesRoot, presentation_, current->boardWidth,
                          current->boardHeight);
        board_.emplace(current->boardWidth, current->boardHeight);
        controller_.emplace(makeController(*board_, commands_, gameView_->geometry, myColor_,
                                           /*interactive=*/myColor_.has_value()));
    }
    syncBoard(*board_, current->pieces);
    if (current->gameOver && !gameOverLogged_) {
        log_.info("game over");
        gameOverLogged_ = true;
    }

    for (const view::MouseEvent& event : window_.takeMouseEvents()) {
        dispatchMouseEvent(event, *controller_);
    }

    std::vector<model::Position> moveTargets;
    if (std::optional<model::Position> selection = controller_->selection()) {
        moveTargets = legalDestinationsFrom(*board_, *selection, current->gameOver);
    }

    std::optional<std::string> overlayText;
    if (std::optional<int> secondsLeft = session_.countdownSecondsLeft()) {
        overlayText = view::countdownText(*secondsLeft);
    }

    window_.show(gameView_->renderer.render(
        view::buildSnapshot(std::move(*current), controller_->selection(),
                            std::move(moveTargets)),
        elapsedMsSince(start_), overlayText));
}

// Once a drag has settled, every screen is rebuilt to the largest size that
// fits where the user left the window. The window itself is deliberately left
// alone: sizing it from here would come straight back through the watcher as
// a fresh resize, rebuilding the view again on a window nobody had touched.
void ClientApp::handleResize() {
    std::optional<view::WindowSize> live = window_.contentSize();
    if (!live) return;

    std::optional<view::WindowSize> settled =
        resizeWatcher_.poll(*live, kFrameDelayMs);
    if (!settled) return;

    int boardCols = gameView_ ? gameView_->geometry.cols() : 0;
    int boardRows = gameView_ ? gameView_->geometry.rows() : 0;

    presentation_ = buildPresentationToFit(kBoardImagePath, *settled);
    lobbyRenderer_ = view::LobbyRenderer{presentation_.layout.canvasWidth(),
                                        presentation_.layout.canvasHeight()};
    roomPromptRenderer_ = view::TextPromptRenderer{presentation_.layout.canvasWidth(),
                                                   presentation_.layout.canvasHeight()};

    if (gameView_) {
        gameView_.emplace(kBoardImagePath, kPiecesRoot, presentation_, boardCols, boardRows);
        controller_->setGeometry(gameView_->geometry);
    }
}

}
