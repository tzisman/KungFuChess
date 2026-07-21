cmake_minimum_required(VERSION 3.20)
project(KungFuChess LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Vendored, header-only test framework (tests/../third_party/doctest/doctest.h).
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# GUI image backend. OpenCV is confined to this target: the pure logic library
# (kfc), the text pipeline, and the tests stay completely free of any display
# dependency, preserving the Business Logic / GUI layer separation.
find_package(OpenCV REQUIRED)

add_library(kfc_img images/img.cpp)
target_include_directories(kfc_img PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/images
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(kfc_img PUBLIC ${OpenCV_LIBS})

# Vendored SQLite amalgamation (single .c/.h pair, same vendoring approach as
# doctest/nlohmann above). Quarantined in its own target so that consumers who
# only need account storage link this directly, without pulling in the rest of
# third_party. SQLITE_THREADSAFE=1 is required because later phases run each
# GameSession on its own thread, all sharing one SQLite connection.
add_library(sqlite3 third_party/sqlite/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite)
target_compile_definitions(sqlite3 PUBLIC SQLITE_THREADSAFE=1)

# Networking transport dependency, vendored header-only under third_party/asio
# and third_party/websocketpp. Quarantined in its own target exactly like OpenCV
# in kfc_img: only kfc_net's own sources see websocketpp/asio (PRIVATE include +
# defines), so the pure logic library kfc stays free of any networking dependency
# and consumers link only against the future net/transport.hpp interface.
add_library(kfc_net
    shared/net/websocketpp_transport.cpp
)
target_include_directories(kfc_net
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/shared
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/websocketpp
)
# _WEBSOCKETPP_CPP11_THREAD_ is required on MinGW: websocketpp otherwise assumes
# MinGW lacks std::thread and falls back to Boost. msys2's mingw64 toolchain
# supports std::thread, so we force the C++11 path and stay Boost-free.
target_compile_definitions(kfc_net PRIVATE
    ASIO_STANDALONE _WEBSOCKETPP_CPP11_STL_ _WEBSOCKETPP_CPP11_THREAD_)
if (WIN32)
    target_compile_definitions(kfc_net PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(kfc_net PUBLIC ws2_32 wsock32 mswsock)
endif()

# Layer-neutral utilities shared by the server and client composition roots.
add_library(kfc_common
    shared/common/logger.cpp
)
target_include_directories(kfc_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/shared)

# All production sources across the layers, shared by the app and the tests.
add_library(kfc
    logic/model/position.cpp
    logic/model/geometry.cpp
    logic/model/piece.cpp
    logic/model/piece_cost.cpp
    logic/model/board.cpp
    logic/rules/piece_rules.cpp
    logic/rules/rule_engine.cpp
    logic/realtime/motion.cpp
    logic/realtime/real_time_arbiter.cpp
    logic/engine/game_engine.cpp
    logic/product/score_board.cpp
    logic/product/move_log.cpp
    logic/product/game_state_view.cpp
    client/input/board_mapper.cpp
    client/input/controller.cpp
    client/view/board_geometry.cpp
    client/view/panel_layout.cpp
    client/view/game_snapshot.cpp
    logic/io/piece_codec.cpp
    logic/io/piece_config.cpp
    logic/io/board_parser.cpp
    logic/io/board_printer.cpp
    logic/io/move_notation.cpp
    texttests/script_parser.cpp
    texttests/script_runner.cpp
)
target_include_directories(kfc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/logic
    ${CMAKE_CURRENT_SOURCE_DIR}/client)
target_include_directories(kfc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Wire protocol shared by server and client: the message types plus their JSON
# encoding. It depends only on the logic library and the vendored JSON header;
# it knows nothing of sockets (net) or display (view), so both sides link it.
add_library(kfc_protocol
    shared/protocol/json_codec.cpp
    shared/protocol/position_codec.cpp
    shared/protocol/wire_snapshot.cpp
)
target_link_libraries(kfc_protocol PUBLIC kfc)
target_include_directories(kfc_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/shared)
target_include_directories(kfc_protocol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Composition root: reads a VPL script on stdin and prints board output.
add_executable(kungfu_chess main.cpp)
target_link_libraries(kungfu_chess PRIVATE kfc)

# The drawing half of the GUI. It is a library of its own so that it can be
# tested, and so that its dependency on the image backend stays visible here
# instead of leaking into the logic library.
add_library(kfc_view
    client/view/sprite_library.cpp
    client/view/renderer.cpp
    client/view/window.cpp
)
target_link_libraries(kfc_view PUBLIC kfc kfc_img)

# Composition root of the GUI: wires the logic, the view and the window. It
# lives at the repository root, alongside the text harness's main, so no main()
# sits inside the display-only view layer.
add_executable(kungfu_chess_gui main_gui.cpp)
target_link_libraries(kungfu_chess_gui PRIVATE kfc_view)

# The authoritative coordination layer. It depends only on the transport
# interface (not the websocketpp implementation), the shared protocol and the
# logger, so its logic is unit-testable with a fake transport.
add_library(kfc_server
    server/command_queue.cpp
    server/player_names.cpp
    server/server_app.cpp
    server/game_session.cpp
    server/elo.cpp
    server/password_hash.cpp
)
target_link_libraries(kfc_server PUBLIC kfc_protocol kfc_common)

# Composition root of the server: joins the server logic to the concrete
# websocketpp transport. Links the networking layer, never the display.
add_executable(kungfu_chess_server main_server.cpp)
target_link_libraries(kungfu_chess_server PRIVATE kfc_server kfc_net)

# Composition root of the networked client: a display-only window fed by the
# server's snapshots. It links the view, the wire protocol and the transport,
# but never the engine — the authoritative game lives on the server.
add_executable(kungfu_chess_client main_client.cpp)
target_link_libraries(kungfu_chess_client PRIVATE kfc_view kfc_protocol kfc_net kfc_common)

# Unit tests. Each tests/unit/test_<module>.cpp is registered here.
add_executable(unit_tests
    tests/test_main.cpp
    tests/unit/test_position.cpp
    tests/unit/test_piece.cpp
    tests/unit/test_piece_cost.cpp
    tests/unit/test_board.cpp
    tests/unit/test_piece_rules.cpp
    tests/unit/test_rule_engine.cpp
    tests/unit/test_real_time_arbiter.cpp
    tests/unit/test_event_bus.cpp
    tests/unit/test_game_engine.cpp
    tests/unit/test_score_board.cpp
    tests/unit/test_move_log.cpp
    tests/unit/test_game_state_view.cpp
    tests/unit/test_board_geometry.cpp
    tests/unit/test_panel_layout.cpp
    tests/unit/test_board_mapper.cpp
    tests/unit/test_controller.cpp
    tests/unit/test_network_command_sink.cpp
    tests/unit/test_game_snapshot.cpp
    tests/unit/test_board_parser.cpp
    tests/unit/test_board_printer.cpp
    tests/unit/test_move_notation.cpp
    tests/unit/test_piece_config.cpp
    tests/unit/test_script_parser.cpp
    tests/unit/test_protocol.cpp
    tests/unit/test_wire_snapshot.cpp
    tests/unit/test_command_queue.cpp
    tests/unit/test_player_names.cpp
    tests/unit/test_server_app.cpp
    tests/unit/test_game_session.cpp
    tests/unit/test_elo.cpp
    tests/unit/test_password_hash.cpp
)
target_link_libraries(unit_tests PRIVATE doctest kfc kfc_protocol kfc_server)

add_test(NAME unit_tests COMMAND unit_tests)
