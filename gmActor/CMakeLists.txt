# ── gmActor – static library ────────────────────────────────────────────────────
#
# Public headers are under gmActor/.  Consumers include them as:
#   #include "gmActor/actors/ActorStore.hpp"
# The workspace root (CMAKE_SOURCE_DIR) is therefore the public include root.
#
# Dependencies:
#   - gmSave   (for gmSave/json.hpp – nlohmann/json – used by ActorJson.cpp)
#   - gmFlow   (for GmFlowActorAdapter.hpp/.cpp)
#
# Both dependencies must be added to the root CMakeLists.txt before gmActor.
# ────────────────────────────────────────────────────────────────────────────────

add_library(gmActor STATIC
    # stats
    stats/Health.cpp
    stats/StatBlock.cpp

    # modifiers
    modifiers/Modifier.cpp

    # statuses
    statuses/StatusContainer.cpp

    # items
    items/InventoryState.cpp
    items/EquipmentState.cpp

    # actors
    actors/ActorStore.cpp
    actors/ActorQueries.cpp

    # serialization (requires gmSave/json.hpp)
    serialization/ActorJson.cpp

    # gmFlow adapter (requires gmFlow headers)
    adapters/GmFlowActorAdapter.cpp
)

target_include_directories(gmActor PUBLIC ${CMAKE_SOURCE_DIR})

# gmActor depends on gmSave for nlohmann/json.hpp and on gmFlow for the adapter.
# Uncomment once both are declared as subdirectories in the root CMakeLists.txt:
# target_link_libraries(gmActor PUBLIC gmSave gmFlow)

# ── Helper: sources shared by all non-serial tests ────────────────────────────

set(GMACTOR_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/stats/Health.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stats/StatBlock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modifiers/Modifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/statuses/StatusContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/items/InventoryState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/items/EquipmentState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/actors/ActorStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/actors/ActorQueries.cpp
)

# ── Test executables ──────────────────────────────────────────────────────────

add_executable(test_gmActor_common
    ${GMACTOR_CORE_SOURCES}
    tests/test_actor_common.cpp
)

add_executable(test_gmActor_health
    ${GMACTOR_CORE_SOURCES}
    tests/test_health.cpp
)

add_executable(test_gmActor_status
    ${GMACTOR_CORE_SOURCES}
    tests/test_status_container.cpp
)

add_executable(test_gmActor_modifier
    modifiers/Modifier.cpp
    tests/test_modifier_container.cpp
)

add_executable(test_gmActor_inventory
    items/InventoryState.cpp
    items/EquipmentState.cpp
    tests/test_inventory_equipment.cpp
)

add_executable(test_gmActor_store
    ${GMACTOR_CORE_SOURCES}
    tests/test_actor_store.cpp
)

# Serialization test (requires gmSave/json.hpp).
add_executable(test_gmActor_serial
    ${GMACTOR_CORE_SOURCES}
    serialization/ActorJson.cpp
    tests/test_serialization.cpp
)

foreach(_t
    test_gmActor_common
    test_gmActor_health
    test_gmActor_status
    test_gmActor_modifier
    test_gmActor_inventory
    test_gmActor_store
    test_gmActor_serial
)
    target_include_directories(${_t} PRIVATE ${CMAKE_SOURCE_DIR})
endforeach()

# ── CTest registration ────────────────────────────────────────────────────────

add_test(NAME gmActor_common    COMMAND test_gmActor_common)
add_test(NAME gmActor_health    COMMAND test_gmActor_health)
add_test(NAME gmActor_status    COMMAND test_gmActor_status)
add_test(NAME gmActor_modifier  COMMAND test_gmActor_modifier)
add_test(NAME gmActor_inventory COMMAND test_gmActor_inventory)
add_test(NAME gmActor_store     COMMAND test_gmActor_store)
add_test(NAME gmActor_serial    COMMAND test_gmActor_serial)

# ── Convenience target ────────────────────────────────────────────────────────

add_custom_target(gmActor_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS
        test_gmActor_common
        test_gmActor_health
        test_gmActor_status
        test_gmActor_modifier
        test_gmActor_inventory
        test_gmActor_store
        test_gmActor_serial
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "=== Running all gmActor tests ==="
    VERBATIM
)

