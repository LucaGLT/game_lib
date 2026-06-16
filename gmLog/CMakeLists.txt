# ── gmLog – static library ────────────────────────────────────────────────────

add_library(gmLog STATIC
    Logger.cpp
    LoggerFactory.cpp
    LogLevel.cpp

    dispatchers/SyncDispatcher.cpp

    formatters/JsonFormatter.cpp

    sinks/FileSink.cpp
    sinks/StdoutSink.cpp
)

target_include_directories(gmLog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)