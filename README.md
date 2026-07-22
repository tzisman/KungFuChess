# KungFuChess

A real-time, **simultaneous** chess variant in C++17 — there are no turns. Both
players move at the same time, every piece must rest before it can move again,
and the game ends when a king is captured (there is no check or checkmate).

The project ships four executables: a headless text pipeline, an offline GUI, a
WebSocket server, and a networked GUI client with accounts, ELO ratings,
matchmaking and named rooms.

## Rules

| Rule | Behaviour |
| --- | --- |
| Simultaneous play | No turns. Both colours issue commands whenever they like. |
| Travel time | A move is not instant: a piece slides cell by cell and occupies the cell it is *leaving* until the hop completes. |
| Cooldown | After arriving, a piece rests (short/long) before it may move again. |
| Jump | A piece can jump in place, staying airborne for a while — a real commitment, since it cannot be commanded meanwhile. |
| Winning | Capturing the king. Nothing else ends the game. |

Timings live in [logic/realtime/motion.hpp](logic/realtime/motion.hpp), and
per-piece speeds are loaded from the `config.json` files under
[images/pieces3/](images/pieces3/).

## Architecture

Three layers, strictly separated — this is the central design constraint:

- **Business Logic** (`logic/`) — piece rules, movement, cooldown, capture and
  win detection. Zero dependency on display or networking.
- **GUI** (`client/`) — draws board state and collects input. Contains no game
  rules.
- **Server** (`server/`) — authoritative coordination between players.

`shared/` holds the contract both sides link: `protocol/` (wire messages + JSON
encoding, depends on `logic` only), `net/` (transport interface + a websocketpp
implementation), `common/` (logger).

Product features observe the engine instead of being called by it:
`GameEngine` publishes `ActionEvent` / `CaptureEvent` / `GameOverEvent` to a
type-indexed `EventBus`, and the score board, move log and renderable snapshot
subscribe to it. The engine knows nothing about who is listening.

Third-party dependencies are quarantined in their own CMake targets so they can
never leak into the logic library: OpenCV in `kfc_img`, websocketpp/asio in
`kfc_net`, SQLite in `kfc_sqlite_store`.

### Layout

```
logic/      model/ rules/ realtime/ bus/ engine/ product/ io/
client/     input/ view/ app/
server/     server_app, session_manager, game_session, matchmaker,
            scheduler, room_registry, elo, sqlite_user_store, ...
shared/     protocol/ net/ common/
texttests/  scripted end-to-end harness + start.txt
images/     OpenCV image backend and piece sprites
tests/unit/ one test file per module
```

See [CLAUDE.md](CLAUDE.md) for the full module map and the code-quality rules
the project is held to.

## Building

Requirements: CMake ≥ 3.20, a C++17 compiler, and OpenCV. Everything else
(doctest, nlohmann/json, asio, websocketpp, SQLite) is vendored under
[third_party/](third_party/).

On Windows with msys2/mingw64:

```sh
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Run the binaries **from the repository root** — they resolve
`texttests/start.txt` and `images/` relative to the working directory.

## Running

| Target | What it does |
| --- | --- |
| `build/kungfu_chess` | Text pipeline: reads a script on stdin, prints boards on stdout. |
| `build/kungfu_chess_gui` | Offline GUI on one machine — both colours playable. |
| `build/kungfu_chess_server` | WebSocket server on port `9002`, accounts in `kungfu_chess.db`. |
| `build/kungfu_chess_client` | Networked client; connects to `ws://127.0.0.1:9002`. |

Online play: start the server, then start one client per player. Each client
registers or logs in, then either presses **PLAY** to be matched by rating or
enters a **room name** to play a friend. A connection with no seat in a room
joins as a spectator and cannot move anything.

GUI controls: click a piece then click a destination to move it, double-click a
piece to jump, **Esc** to quit. The window is resizable — the board and side
panels re-layout once the size settles.

## Text scripts

`kungfu_chess` reads a board followed by commands, the format used by
[texttests/demo.vpl](texttests/demo.vpl):

```
Board:
bR bN bB bQ bK bB bN bR
bP bP bP bP bP bP bP bP
. . . . . . . .
...
Commands:
print board
click 450 650
click 450 450
wait 5000
print board
```

Commands are `click <x> <y>`, `jump <x> <y>`, `wait <ms>` and `print board`.
Coordinates are pixels on a fixed 100px grid, so the script drives the same
`Controller` the GUI does — the seam between GUI and Logic is exercised without
opening a window.

```sh
./build/kungfu_chess < texttests/demo.vpl
```

## Tests

All tests use [doctest](https://github.com/doctest/doctest), one file per module
under [tests/unit/](tests/unit/), covering the logic, the GUI–logic seam, the
protocol and the server.

```sh
ctest --test-dir build --output-on-failure
# or run the binary directly:
./build/unit_tests
```
