# ── Dungeon Crawler Basic CoreEngine – C++17 executable ─────────────────────
#
# Reuses the existing gmXxx libraries:
#   - gmDispatch : outbound GUI events (IpSocketChannel) + wire format
#   - gmLog      : session logging
#   - gmAlea     : random dice (future use)
#   - gmSave     : header-only nlohmann/json (gmSave/json.hpp)
#   - gmFlow     : turn/round management
#   - gmRules    : rule engine for action validation
#   - gmMap      : dungeon map (header-only template, via include path)
#
# Added from the workspace-root CMakeLists.txt with:
#   add_subdirectory(GAME/Dungeon-Crawler-Basic/CoreEngine)
# so ${CMAKE_SOURCE_DIR} points at the workspace root and the gmXxx targets
# are already available to link against.
# ────────────────────────────────────────────────────────────────────────────

add_executable(dungeon_engine
    main.cpp

    engine/DungeonEngine.cpp

    world/DungeonMap.cpp
    world/DungeonMapLoader.cpp
    actors/ActorRoster.cpp
    flow/TurnFlow.cpp
    rules/DungeonRuleAdapter.cpp
    actions/ActionV1.cpp
    log/GameLog.cpp

    bridge/GuiBridge.cpp
    bridge/CmdServer.cpp

    # gmActor core sources (no serialization/gmFlow adapter).
    ${CMAKE_SOURCE_DIR}/gmActor/stats/Health.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/stats/StatBlock.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/modifiers/Modifier.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/statuses/StatusContainer.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/InventoryState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorStore.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorQueries.cpp
)

target_include_directories(dungeon_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # local: engine/, world/, bridge/, …
    ${CMAKE_SOURCE_DIR}           # gmXxx public headers (gmDispatch/, gmSave/, …)
)

# gmSave/gmSave.hpp static_asserts on __cplusplus; MSVC only reports it
# correctly with /Zc:__cplusplus.
if(MSVC)
    target_compile_options(dungeon_engine PRIVATE /Zc:__cplusplus)
endif()

target_link_libraries(dungeon_engine PRIVATE
    gmDispatch
    gmLog
    gmAlea
    gmFlow
    gmRules
)

# Winsock2 is required by IpSocketChannel and CmdServer on Windows.
if(WIN32)
    target_link_libraries(dungeon_engine PRIVATE ws2_32)
endif()
