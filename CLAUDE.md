# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Status

The project is currently undergoing a **major refactoring of the entire folder/directory structure**, restructuring the codebase around explicit **design patterns** and the strict layer separation described below. Expect files and directories to be reorganized; when adding or moving code, follow the intended architecture and design-pattern conventions rather than the previous layout. The sections below describe the **intended** architecture and rules the implementation must follow.

The target layout (see **Project Structure** below) is now the sole layout: the source is grouped into four top-level umbrellas that mirror the architectural layers — `logic/` (Business Logic), `client/` (GUI), `server/` (Server), and `shared/` (the networking contract both sides depend on) — with `tests/` mirroring them. The legacy `App/` and `Business Logic/` directories have been fully migrated and removed.

## Game Overview

KungFuChess is a **real-time, simultaneous** chess variant — there are no turns. Both players move at the same time. Core rules that the logic must enforce:

- After any move, a piece must **rest (cooldown)** before it can move again.
- **Winning = capturing the king.** There is no concept of check or checkmate.
- Extra piece commands: **Jump** (move + short pause).
- Extra piece types beyond standard chess, e.g. **Quadcopter**: slower cooldown than other pieces, but can move to any square in the `[±2, ±2]` range.
- Product features layered on top: **moves log**, **score** (sum of the "cost" of captured pieces), and **player name** display.

## Architecture — the central constraint

The project is organized into three layers with **strict separation**. This is the single most important design rule and the primary thing the project is graded on — quality of design and layer separation, not merely "does it work."

- **Business Logic** — the heart of the game, with **zero dependency on display or networking**: piece rules, movement, cooldown, capture logic, and win detection live here and nowhere else.
- **GUI** — display only. Renders board state and collects input. Contains **no game rules**.
- **Server** — the networking / coordination layer between players.

The guiding principle: **Business Logic must be completely decoupled from GUI and Server.** Never mix game rules into display or network code. If the layers are designed this way from the start, the test structure and separation fall into place almost automatically. When making changes, treat any leak of game rules into the GUI or Server layer as a design defect to be corrected, not accommodated.

A non-functional/aspirational goal is scalability toward millions of concurrent players. This is architectural pressure to keep layers cleanly separated and the server well-designed — not a mandate to actually implement at that scale.

## Project Structure

The codebase is being migrated to the layout below. The source modules live at the repository root, each mapping onto one of the architectural layers; `tests/` mirrors them. Add new code to the module that matches its responsibility — never widen a module's job to avoid creating the right one.

The source is grouped into four top-level umbrellas that mirror the architectural
layers — `logic/` (Business Logic), `client/` (GUI), `server/` (Server), and
`shared/` (the networking contract both sides depend on). Every `#include` uses
the module-prefixed form (`"model/board.hpp"`, `"protocol/messages.hpp"`); the
umbrella directories are on the include path, so the prefix names the module, not
the umbrella.

```
logic/                  # Business Logic — zero dependency on display or networking
  model/                # pure domain data & state
    position            # a board coordinate
    piece               # piece identity (type, color, name)
    piece_cost          # what a piece is worth to whoever captures it
    board               # the grid of pieces + safe accessors
    game_state          # authoritative game state + game clock (no orchestration)
  rules/                # the rules of the game
    piece_rules         # per-type movement/legality rules
    rule_engine         # orchestrates the rules into a legality decision
  realtime/             # the real-time / simultaneous mechanics
    motion              # travel time, cooldown, jump durations
    real_time_arbiter   # pending moves/jumps, the clock, arrival resolution
  bus/                  # messaging infrastructure (layer-neutral)
    event_bus           # generic type-indexed publish/subscribe bus
  engine/               # top-level coordination
    game_engine         # drives state + rules + realtime; the logic entry point
                        # and the Subject that publishes what happened to the bus
    game_events         # the game's event types (Action/Capture/GameOver)
  product/              # product features built on game events
    score_board         # subscriber: each player's score
    move_log            # subscriber: each player's actions, as records
    game_state_view     # subscriber: renderable snapshot of the game
  io/                   # serialization — board text in/out (not display, not rules)
    board_parser        # text -> board + commands
    board_printer       # board -> text
    move_notation       # a logged action -> text (square names, clock)
    piece_codec         # piece <-> letter/color encoding
    piece_config        # piece-type table (letters, costs, motion)
client/                 # GUI — display & input only, no game rules
  input/                # GUI (input side)
    board_mapper        # pixel <-> cell mapping (display-coupled)
    controller          # turns raw input into engine commands
    command_sink        # where a controller sends commands (engine or network)
  view/                 # GUI (output side)
    renderer            # renders board state
    window              # the graphical window
    sprite_library      # piece images
    board_geometry      # cell <-> pixel, and where the board sits on the canvas
    panel_layout        # where the board and the side panels sit
  app/                  # composition helpers shared by the GUI/client roots
server/                 # Server — authoritative coordination between players
  server_app            # accepts connections, drives sessions
  game_session          # one game's server-side state & clients
  command_queue         # queued player intents
  player_names          # per-connection player identity
shared/                 # networking contract shared by client and server
  protocol/             # wire messages + their JSON encoding (depends on logic only)
    messages  json_codec  position_codec  wire_snapshot
  net/                  # transport interface + websocketpp implementation
    transport  websocketpp_transport
  common/               # layer-neutral utilities (logger)
texttests/              # scripted end-to-end test harness (script_parser, script_runner)
images/                 # OpenCV image backend, quarantined from the logic library
main.cpp                # text-pipeline composition root
main_gui.cpp            # offline GUI composition root
main_server.cpp         # server composition root
main_client.cpp         # networked-client composition root

tests/
  unit/                 # Business Logic + seams, each unit tested in isolation
    test_position  test_board  test_piece_rules  test_rule_engine
    test_real_time_arbiter  test_event_bus  test_game_engine  test_board_mapper
    test_controller  test_board_parser  test_board_printer  test_piece_cost
    test_score_board  test_move_log  test_move_notation  test_panel_layout
    test_protocol  test_wire_snapshot  test_server_app  test_game_session ...
```

Layer mapping: everything under `logic/` is **Business Logic**; `client/` is the
**GUI**; `server/` is the **Server**. `shared/` holds the networking contract both
the client and server link — `protocol` is a serialization boundary (it depends on
`logic` only, never on sockets or display), `net` is the transport, `common` is
layer-neutral. Business Logic must never `#include` from `client/` or `shared/`.

Product features (score, moves log) observe the engine rather than being called by it: `GameEngine` publishes `ActionEvent` / `CaptureEvent` / `GameOverEvent` to a generic `EventBus`, and knows nothing of who is listening. Subscribers register through `engine.events()` and each declares what it cares about via its own `subscribeTo(bus)`. A feature that accumulates from game events belongs in `product/` as a subscriber — never as a hook inside the rules, and never in the GUI. The bus is deliberately ignorant of any concrete event type, so it depends on no game layer: this is the seam through which the future Server layer will publish and subscribe without the logic ever hearing of it.

### Naming conventions

- **Files & directories:** `snake_case`, one module per name, paired `.hpp` / `.cpp` (header-only modules may omit the `.cpp`). Test files are `.cpp` only, named `test_<module>`.
- **Types (classes, structs, enums):** `PascalCase`.
- **Functions & variables:** `camelCase`; private members carry a trailing underscore (`board_`).
- **Constants / enum values:** `kPascalCase` (e.g. `kSquareTravelMs`).
- **Namespaces:** `kfc` at the root, one nested namespace per layer (`kfc::model`, `kfc::rules`, `kfc::realtime`, `kfc::bus`, `kfc::engine`, `kfc::product`, `kfc::input`, `kfc::view`, `kfc::io`). Business Logic namespaces must never `#include` from `input`/`view`.

## Code Quality Rules

Beyond layer separation, all code — Business Logic above all — must follow:

- **DRY** — every piece of logic/domain knowledge is implemented in exactly one place. If a fact (e.g. "which letters are valid piece types") is needed in two places, define it once and reference it from both; do not let two independent switch/if chains encode the same rule, since they will drift out of sync.
- **SRP** — every function does exactly one thing. A function that validates, parses, transforms, and mutates state in one body should be split into named steps, each doing one of those things.
- **No hardcoded constants or strings in Business Logic** — piece-type letters, color markers, the empty-cell token, board dimensions, cooldown durations, piece costs, etc. must live in named constants/enums/config, never as inline literals (`'K'`, `"."`, `'w'`, `100`) scattered through the logic.
- **Encapsulation** — classes and functions expose behavior, not internal representation. Don't return raw internal containers (e.g. a `vector<vector<string>>` board) from an accessor and let callers pattern-match against the encoding; expose purpose-built methods instead.
- **Minimal comments** — write as few comments as possible; let clear names and small, single-purpose functions carry the meaning. Add a comment only when it explains *why* something non-obvious is done, never to restate *what* the code already says. Prefer renaming or refactoring over adding an explanatory comment.

Treat a violation of any of these as a design defect to fix, not a style nit — review new/changed code against this list before considering a change complete.

## Testing strategy

The test framework is **[doctest](https://github.com/doctest/doctest)** — all unit tests under `tests/` are written with it.

Each layer is verified by tests appropriate to it:

- **Unit Tests** — Business Logic in isolation.
- **GUI–Logic Unit Tests** — the seam between GUI and Logic.
- **Integration Tests** — at the server level.
- **Acceptance Tests** — product requirements verified end-to-end (Logic + GUI).
- **System Tests** — the full system including the server.

When implementing a feature, add or update tests at the layer(s) it touches; a change to a game rule belongs in Business Logic and its unit tests, never validated only through the GUI.
