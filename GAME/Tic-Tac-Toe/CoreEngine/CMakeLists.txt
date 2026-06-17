# ── Tic-Tac-Toe CoreEngine – C++17 executable ───────────────────────────────
#
# Reuses the existing gmXxx libraries:
#   - gmDispatch : outbound GUI events (IpSocketChannel) + wire format
#   - gmLog      : match logging
#   - gmAlea     : 1d2 starter die
#   - gmSave     : header-only nlohmann/json (gmSave/json.hpp)
#
# This file is meant to be added from the workspace-root CMakeLists.txt with:
#   add_subdirectory(GAME/Tic-Tac-Toe/CoreEngine)
# so that ${CMAKE_SOURCE_DIR} points at the workspace root and the gmXxx
# targets are available to link against.
# ────────────────────────────────────────────────────────────────────────────

add_executable(tris_engine
    main.cpp

    engine/TrisEngine.cpp

    board/Board.cpp
    players/Players.cpp
    flow/TurnFlow.cpp
    rules/WinRules.cpp
    rules/TrisRuleContext.cpp
    alea/Starter.cpp
    logging/GameLog.cpp

    bridge/GuiBridge.cpp
    bridge/CmdServer.cpp

    # gmActor core sources (no serialization/gmFlow adapter): backs Players.
    ${CMAKE_SOURCE_DIR}/gmActor/stats/Health.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/stats/StatBlock.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/modifiers/Modifier.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/statuses/StatusContainer.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/InventoryState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorStore.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorQueries.cpp
)

target_include_directories(tris_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # local headers: engine/, board/, bridge/, ...
    ${CMAKE_SOURCE_DIR}           # gmXxx public headers (gmDispatch/, gmSave/, ...)
)

# gmSave/gmSave.hpp static_asserts on __cplusplus, which MSVC only reports
# correctly when /Zc:__cplusplus is enabled.
if(MSVC)
    target_compile_options(tris_engine PRIVATE /Zc:__cplusplus)
endif()

target_link_libraries(tris_engine PRIVATE
    gmDispatch
    gmLog
    gmAlea
    gmFlow
    gmRules
)

# Winsock2 is required by IpSocketChannel and by our CmdServer on Windows.
if(WIN32)
    target_link_libraries(tris_engine PRIVATE ws2_32)
endif()

# ── Phase 4 – tests ─────────────────────────────────────────────────────────
# gmActor core sources needed to back the Players wrapper in the unit tests.
set(TRIS_GMACTOR_CORE
    ${CMAKE_SOURCE_DIR}/gmActor/stats/Health.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/stats/StatBlock.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/modifiers/Modifier.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/statuses/StatusContainer.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/InventoryState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorStore.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorQueries.cpp
)

# Helper that wires the shared include dirs and the MSVC __cplusplus flag.
function(tris_configure_test target)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE /Zc:__cplusplus)
    endif()
endfunction()

# 1) Move mechanics + win/draw rules (Board, WinRules over gmMap/gmRules).
add_executable(test_tris_rules
    tests/test_rules.cpp
    board/Board.cpp
    rules/WinRules.cpp
    rules/TrisRuleContext.cpp
)
tris_configure_test(test_tris_rules)
target_link_libraries(test_tris_rules PRIVATE gmRules)

# 2) Player identity/statuses (gmActor) + starter logic (gmAlea).
add_executable(test_tris_players
    tests/test_players.cpp
    players/Players.cpp
    alea/Starter.cpp
    ${TRIS_GMACTOR_CORE}
)
tris_configure_test(test_tris_players)
target_link_libraries(test_tris_players PRIVATE gmFlow gmAlea)

# 3) Inbound command bridge robustness (valid / malformed frames, reconnect).
add_executable(test_tris_bridge
    tests/test_cmdserver.cpp
    bridge/CmdServer.cpp
)
tris_configure_test(test_tris_bridge)
if(WIN32)
    target_link_libraries(test_tris_bridge PRIVATE ws2_32)
endif()
find_package(Threads REQUIRED)
target_link_libraries(test_tris_bridge PRIVATE Threads::Threads)

# Register the C++ tests with CTest.
add_test(NAME tris_rules   COMMAND test_tris_rules)
add_test(NAME tris_players COMMAND test_tris_players)
add_test(NAME tris_bridge  COMMAND test_tris_bridge)

# 4) End-to-end integration test driving the real engine over TCP (Python).
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME tris_e2e
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../GUI/tests/e2e_test.py
                $<TARGET_FILE:tris_engine>
    )
    # The E2E test launches tris_engine itself; make sure it is built first.
    set_tests_properties(tris_e2e PROPERTIES
        DEPENDS "tris_rules;tris_players;tris_bridge"
        TIMEOUT 60
    )
endif()
