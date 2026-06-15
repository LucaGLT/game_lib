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
# Link order: gmActor → gmSave, gmFlow must be declared before this subdirectory.
# target_link_libraries(gmActor PUBLIC gmSave gmFlow)
# Uncomment the line above once gmSave and gmFlow are added as subdirectories
# in the root CMakeLists.txt.
