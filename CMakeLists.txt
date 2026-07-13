cmake_minimum_required(VERSION 3.20)
project(KungFuChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Vendored, header-only test framework (tests/../third_party/doctest/doctest.h).
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Unit tests. As each Business-Logic module is migrated into the new layout,
# add its source and matching tests/unit/test_<module>.cpp to this target.
add_executable(unit_tests
    tests/test_main.cpp
    tests/unit/test_position.cpp
    tests/unit/test_piece.cpp
    tests/unit/test_board.cpp
    tests/unit/test_piece_rules.cpp
    tests/unit/test_rule_engine.cpp
    tests/unit/test_real_time_arbiter.cpp
    model/position.cpp
    model/piece.cpp
    model/board.cpp
    rules/piece_rules.cpp
    rules/rule_engine.cpp
    realtime/motion.cpp
    realtime/real_time_arbiter.cpp
)
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unit_tests PRIVATE doctest)

add_test(NAME unit_tests COMMAND unit_tests)
