# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Status

Greenfield project — no source code, build system, or tests exist yet. The sections below describe the **intended** architecture and rules the implementation must follow.

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

## Code Quality Rules

Beyond layer separation, all code — Business Logic above all — must follow:

- **DRY** — every piece of logic/domain knowledge is implemented in exactly one place. If a fact (e.g. "which letters are valid piece types") is needed in two places, define it once and reference it from both; do not let two independent switch/if chains encode the same rule, since they will drift out of sync.
- **SRP** — every function does exactly one thing. A function that validates, parses, transforms, and mutates state in one body should be split into named steps, each doing one of those things.
- **No hardcoded constants or strings in Business Logic** — piece-type letters, color markers, the empty-cell token, board dimensions, cooldown durations, piece costs, etc. must live in named constants/enums/config, never as inline literals (`'K'`, `"."`, `'w'`, `100`) scattered through the logic.
- **Encapsulation** — classes and functions expose behavior, not internal representation. Don't return raw internal containers (e.g. a `vector<vector<string>>` board) from an accessor and let callers pattern-match against the encoding; expose purpose-built methods instead.

Treat a violation of any of these as a design defect to fix, not a style nit — review new/changed code against this list before considering a change complete.

## Testing strategy

Each layer is verified by tests appropriate to it:

- **Unit Tests** — Business Logic in isolation.
- **GUI–Logic Unit Tests** — the seam between GUI and Logic.
- **Integration Tests** — at the server level.
- **Acceptance Tests** — product requirements verified end-to-end (Logic + GUI).
- **System Tests** — the full system including the server.

When implementing a feature, add or update tests at the layer(s) it touches; a change to a game rule belongs in Business Logic and its unit tests, never validated only through the GUI.
