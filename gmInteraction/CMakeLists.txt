# ── gmInteraction – game-independent interactable object registry ────────────
#
# Standalone C++17 library. Owns interactable object *data* (type, state,
# metadata). Spatial placement lives in gmMap; the two are linked through the
# header-only bridge in bridges/MapInteractionBridge.hpp.
#
# Depends on gmSave for versioned JSON snapshots.
# ─────────────────────────────────────────────────────────────────────────────

add_library(gmInteraction STATIC
    gmInteraction.cpp
    InteractableObjectStore.cpp
)

target_include_directories(gmInteraction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(gmInteraction PUBLIC gmSave)

# gmSave/gmSave.hpp static_asserts on __cplusplus; MSVC needs /Zc:__cplusplus.
if(MSVC)
    target_compile_options(gmInteraction PUBLIC /Zc:__cplusplus)
endif()

# ── Unit test ────────────────────────────────────────────────────────────────

add_executable(test_gmInteraction tests/test_gmInteraction.cpp)
target_include_directories(test_gmInteraction PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(test_gmInteraction PRIVATE gmInteraction gmMap)
if(MSVC)
    target_compile_options(test_gmInteraction PRIVATE /Zc:__cplusplus)
endif()

add_test(NAME test_gmInteraction COMMAND test_gmInteraction
         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
