// Single translation unit that provides doctest's implementation and main().
// Every other test_*.cpp only includes the doctest header and registers cases.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
