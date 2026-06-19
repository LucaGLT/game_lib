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
    gmMap
    gmInteraction
)

# Winsock2 is required by IpSocketChannel and CmdServer on Windows.
if(WIN32)
    target_link_libraries(dungeon_engine PRIVATE ws2_32)
endif()

# ── Shared sources compiled into every unit test ──────────────────────────────

set(_DUNGEON_GMACTOR_SRCS
    ${CMAKE_SOURCE_DIR}/gmActor/stats/Health.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/stats/StatBlock.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/modifiers/Modifier.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/statuses/StatusContainer.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/InventoryState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/items/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorStore.cpp
    ${CMAKE_SOURCE_DIR}/gmActor/actors/ActorQueries.cpp
)

set(_DUNGEON_CORE_SRCS
    world/DungeonMap.cpp
    world/DungeonMapLoader.cpp
    actors/ActorRoster.cpp
    flow/TurnFlow.cpp
    rules/DungeonRuleAdapter.cpp
    actions/ActionV1.cpp
    log/GameLog.cpp
    ${_DUNGEON_GMACTOR_SRCS}
)

set(_DUNGEON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(_DUNGEON_LINK_LIBS
    gmDispatch
    gmLog
    gmAlea
    gmFlow
    gmRules
)

if(WIN32)
    list(APPEND _DUNGEON_LINK_LIBS ws2_32)
endif()

# ── Macro for registering a unit test ────────────────────────────────────────

function(dungeon_add_test target src)
    add_executable(${target} tests/${src} ${_DUNGEON_CORE_SRCS})
    target_include_directories(${target} PRIVATE ${_DUNGEON_INCLUDE_DIRS})
    target_link_libraries(${target} PRIVATE ${_DUNGEON_LINK_LIBS})
    if(MSVC)
        target_compile_options(${target} PRIVATE /Zc:__cplusplus)
    endif()
    add_test(NAME ${target} COMMAND ${target}
             WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endfunction()

# ── Register unit tests ───────────────────────────────────────────────────────

enable_testing()

dungeon_add_test(test_dungeon_map      test_dungeon_map.cpp)
dungeon_add_test(test_actor_roster     test_actor_roster.cpp)
dungeon_add_test(test_turn_flow        test_turn_flow.cpp)
dungeon_add_test(test_dungeon_rules    test_dungeon_rules.cpp)
dungeon_add_test(test_map_loader       test_map_loader.cpp)

# ── Python tests via CTest ────────────────────────────────────────────────────

find_program(PYTHON_EXECUTABLE NAMES python3 python)
if(PYTHON_EXECUTABLE)
    set(_GUI_TESTS_DIR "${CMAKE_SOURCE_DIR}/GAME/Dungeon-Crawler-Basic/GUI/tests")

    add_test(
        NAME   dungeon_py_test_event_router
        COMMAND "${PYTHON_EXECUTABLE}" "${_GUI_TESTS_DIR}/test_event_router.py"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    add_test(
        NAME   dungeon_py_test_widgets
        COMMAND "${PYTHON_EXECUTABLE}" "${_GUI_TESTS_DIR}/test_widgets.py"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(
        dungeon_py_test_event_router
        dungeon_py_test_widgets
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
