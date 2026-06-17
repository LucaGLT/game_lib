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
    alea/Starter.cpp
    logging/GameLog.cpp

    bridge/GuiBridge.cpp
    bridge/CmdServer.cpp
)

target_include_directories(tris_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # local headers: engine/, board/, bridge/, ...
    ${CMAKE_SOURCE_DIR}           # gmXxx public headers (gmDispatch/, gmSave/, ...)
)

target_link_libraries(tris_engine PRIVATE
    gmDispatch
    gmLog
    gmAlea
)

# Winsock2 is required by IpSocketChannel and by our CmdServer on Windows.
if(WIN32)
    target_link_libraries(tris_engine PRIVATE ws2_32)
endif()
