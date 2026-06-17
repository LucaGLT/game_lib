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
