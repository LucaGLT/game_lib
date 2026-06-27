# ── Le Pergamene di Eldhôm — CoreEngine ─────────────────────────────────────
#
# C++17 executable (+ unit tests) for the Eldhom game engine.
#
# Reuses the following workspace-root libraries:
#   - gmAlea   : SequenceEngine (card sequences)
#   - gmActor  : ActorStore, FormationValidator, BehaviorCardProcessor
#   - gmLog    : session logging
#   - gmSave   : header-only nlohmann/json (gmSave/json.hpp)
#
# Include paths:
#   ${CMAKE_CURRENT_SOURCE_DIR}    — GAME/Eldhom/CoreEngine/ prefix
#   ${CMAKE_SOURCE_DIR}            — workspace root  (gmAlea/, gmActor/, …)
#
# Added from the workspace-root CMakeLists.txt with:
#   add_subdirectory(GAME/Eldhom/CoreEngine)
# ─────────────────────────────────────────────────────────────────────────────

# ── CoreEngine sources ────────────────────────────────────────────────────────

set(_ELDHOM_ENGINE_SRCS
    engine/EldhomEngine.cpp

    sequence/EldhomSequenceAdapter.cpp
    formation/EldhomFormationAdapter.cpp
    monsters/EldhomBehaviorAdapter.cpp
    mission/MissionEventSystem.cpp
    targeting/TargetingFilter.cpp
    rules/EldhomRuleAdapter.cpp
)

# ── eldhom_engine — standalone executable (entry point for CLI / tests) ───────

add_executable(eldhom_engine
    main.cpp
    ${_ELDHOM_ENGINE_SRCS}
)

target_include_directories(eldhom_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # GAME/Eldhom/CoreEngine/ — local headers
    ${CMAKE_SOURCE_DIR}           # workspace root — gmAlea/, gmActor/, …
)

target_link_libraries(eldhom_engine PRIVATE
    gmAlea
    gmActor
    gmLog
)

if(MSVC)
    target_compile_options(eldhom_engine PRIVATE
        /Zc:__cplusplus      # correct __cplusplus value for gmSave
        /W4                  # high warning level
    )
endif()

# ── Unit tests ────────────────────────────────────────────────────────────────

set(_ELDHOM_TEST_SHARED_SRCS
    ${_ELDHOM_ENGINE_SRCS}
)

# Helper macro: add one Eldhom unit test executable
macro(eldhom_add_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME}
        tests/${TEST_SRC}
        ${_ELDHOM_TEST_SHARED_SRCS}
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(${TEST_NAME} PRIVATE gmAlea gmActor gmLog)
    if(MSVC)
        target_compile_options(${TEST_NAME} PRIVATE /Zc:__cplusplus /W4)
    endif()
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# Test targets (created after tests/ files are written)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_eldhom_engine.cpp)
    eldhom_add_test(test_eldhom_engine test_eldhom_engine.cpp)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_eldhom_mission01.cpp)
    eldhom_add_test(test_eldhom_mission01 test_eldhom_mission01.cpp)
endif()
