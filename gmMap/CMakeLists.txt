# ── gmMap – generic, topology-agnostic game map (header-only template) ───────
#
# gmMap<ItemT> is a header-only class template, so this is an INTERFACE target:
# it carries the include path and the gmSave dependency (gmMap.hpp includes
# gmSave/gmSave.hpp) without compiling any translation unit of its own.
# ─────────────────────────────────────────────────────────────────────────────

add_library(gmMap INTERFACE)

target_include_directories(gmMap INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(gmMap INTERFACE gmSave)

if(MSVC)
    target_compile_options(gmMap INTERFACE /Zc:__cplusplus)
endif()
