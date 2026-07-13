# ── gmAlea – static library ────────────────────────────────────────────────────
#
# All public headers are under gmAlea/. Consumers include them as:
#   #include "gmAlea/GmDeck.hpp"
# The workspace root is therefore the public include root (via CMAKE_SOURCE_DIR).
# ────────────────────────────────────────────────────────────────────────────────

add_library(gmAlea STATIC
    GmDeck.cpp
    GmCompDeck.cpp
    SimpleDeck.cpp
    GmDice.cpp
    StdDice.cpp
    SequenceEngine.cpp
)

target_include_directories(gmAlea PUBLIC ${CMAKE_SOURCE_DIR})

# ── Test executables ────────────────────────────────────────────────────────────
# test_gmDeck_v2 and test_gmCompDeck include headers as "gmAlea/Foo.hpp" (root-based).
# test_gmDice and test_stdDice include as "../Foo.hpp" (source-file-relative).
# Both styles resolve correctly: the root include dir is inherited from gmAlea.

add_executable(test_gmDeck_v2      tests/test_gmDeck_v2.cpp)
add_executable(test_gmCompDeck     tests/test_gmCompDeck.cpp)
add_executable(test_gmDice         tests/test_gmDice.cpp)
add_executable(test_stdDice        tests/test_stdDice.cpp)
add_executable(test_SequenceEngine tests/test_SequenceEngine.cpp)

foreach(_t test_gmDeck_v2 test_gmCompDeck test_gmDice test_stdDice test_SequenceEngine)
    target_link_libraries(${_t} PRIVATE gmAlea)
endforeach()

# ── CTest registration ──────────────────────────────────────────────────────────
# Run with: ctest --output-on-failure   (from the build directory)

add_test(NAME gmDeck_v2      COMMAND test_gmDeck_v2)
add_test(NAME gmCompDeck     COMMAND test_gmCompDeck)
add_test(NAME gmDice         COMMAND test_gmDice)
add_test(NAME stdDice        COMMAND test_stdDice)
add_test(NAME SequenceEngine COMMAND test_SequenceEngine)

# ── Convenience target: gmAlea_all_tests ────────────────────────────────────────
# Builds all four test executables, then runs them via ctest.
#
# Usage:
#   cmake --build <build-dir> --target gmAlea_all_tests
#   make gmAlea_all_tests          (Makefile/Ninja generators)

add_custom_target(gmAlea_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_gmDeck_v2 test_gmCompDeck test_gmDice test_stdDice test_SequenceEngine
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "=== Running all gmAlea tests ==="
    VERBATIM
)
