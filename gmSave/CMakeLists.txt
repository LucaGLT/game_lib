# ── gmSave – versioned JSON persistence (nlohmann/json wrapper) ──────────────
#
# Compiled as a static library so the non-template helpers (save_versioned /
# load_versioned / peek_version implementation details) are built once and
# linked by every consumer (gmMap, gmInteraction, …).
# ─────────────────────────────────────────────────────────────────────────────

add_library(gmSave STATIC gmSave.cpp)

target_include_directories(gmSave PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# gmSave/gmSave.hpp static_asserts on __cplusplus; MSVC needs /Zc:__cplusplus.
if(MSVC)
    target_compile_options(gmSave PUBLIC /Zc:__cplusplus)
endif()
