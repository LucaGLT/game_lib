cmake_minimum_required(VERSION 3.15)

project(game_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable CTest at the root so `ctest` works from the build directory.
enable_testing()

# ── Libraries ──────────────────────────────────────────────────────────────────
add_subdirectory(gmAlea)

# Other libraries (gmDispatch, gmLog, gmMap, …) can be added here when ready.
# add_subdirectory(gmDispatch)
# add_subdirectory(gmLog)
# add_subdirectory(gmMap)
add_subdirectory(gmRules)
# add_subdirectory(gmSave)
