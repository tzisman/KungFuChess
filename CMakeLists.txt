cmake_minimum_required(VERSION 3.20)
project(KungFuChess LANGUAGES CXX)

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

# Networking transport dependency, vendored header-only under third_party/asio
# and third_party/websocketpp. Quarantined in its own target exactly like OpenCV
# in kfc_img: only kfc_net's own sources see websocketpp/asio (PRIVATE include +
# defines), so the pure logic library kfc stays free of any networking dependency
# and consumers link only against the future net/transport.hpp interface.
add_library(kfc_net
    net/net_deps_check.cpp
)
target_include_directories(kfc_net
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
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
    target_link_libraries(kfc_net PUBLIC ws2_32 wsock32)
endif()

# All production sources across the layers, shared by the app and the tests.
add_library(kfc
    model/position.cpp
    model/geometry.cpp
    model/piece.cpp
    model/piece_cost.cpp
    model/board.cpp
    rules/piece_rules.cpp
    rules/rule_engine.cpp
    realtime/motion.cpp
    realtime/real_time_arbiter.cpp
    engine/game_engine.cpp
    product/score_board.cpp
    product/move_log.cpp
    product/game_state_view.cpp
    input/board_mapper.cpp
    input/controller.cpp
    view/board_geometry.cpp
    view/panel_layout.cpp
    view/game_snapshot.cpp
    io/piece_codec.cpp
    io/piece_config.cpp
    io/board_parser.cpp
    io/board_printer.cpp
    io/move_notation.cpp
    texttests/script_parser.cpp
    texttests/script_runner.cpp
)
target_include_directories(kfc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(kfc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Wire protocol shared by server and client: the message types plus their JSON
# encoding. It depends only on the logic library and the vendored JSON header;
# it knows nothing of sockets (net) or display (view), so both sides link it.
add_library(kfc_protocol
    protocol/json_codec.cpp
)
target_link_libraries(kfc_protocol PUBLIC kfc)
target_include_directories(kfc_protocol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Composition root: reads a VPL script on stdin and prints board output.
add_executable(kungfu_chess main.cpp)
target_link_libraries(kungfu_chess PRIVATE kfc)

# The drawing half of the GUI. It is a library of its own so that it can be
# tested, and so that its dependency on the image backend stays visible here
# instead of leaking into the logic library.
add_library(kfc_view
    view/sprite_library.cpp
    view/renderer.cpp
    view/window.cpp
)
target_link_libraries(kfc_view PUBLIC kfc kfc_img)

# Composition root of the GUI: wires the logic, the view and the window. It
# lives at the repository root, alongside the text harness's main, so no main()
# sits inside the display-only view layer.
add_executable(kungfu_chess_gui main_gui.cpp)
target_link_libraries(kungfu_chess_gui PRIVATE kfc_view)

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
    tests/unit/test_game_snapshot.cpp
    tests/unit/test_board_parser.cpp
    tests/unit/test_board_printer.cpp
    tests/unit/test_move_notation.cpp
    tests/unit/test_piece_config.cpp
    tests/unit/test_script_parser.cpp
    tests/unit/test_protocol.cpp
)
target_link_libraries(unit_tests PRIVATE doctest kfc kfc_protocol)

add_test(NAME unit_tests COMMAND unit_tests)
