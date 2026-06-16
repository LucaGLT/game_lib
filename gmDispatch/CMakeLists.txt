# ── gmDispatch – static library ────────────────────────────────────────────────

add_library(gmDispatch STATIC
    Dispatcher.cpp
    DispatcherFactory.cpp

    bridges/LogDispatchBridge.cpp
    bridges/DispatchRuleContextAdapter.cpp
    bridges/RuleEventBridge.cpp

    channels/EventBusChannel.cpp
    channels/FileChannel.cpp
    channels/IpSocketChannel.cpp
    channels/StdoutChannel.cpp

    dispatchers/AsyncDispatcher.cpp
    dispatchers/SyncDispatcher.cpp

    routers/PatternRouter.cpp
    routers/SyncRouter.cpp

    serializers/JsonSerializer.cpp
)

target_include_directories(gmDispatch PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_executable(test_gmDispatch_smoke_phase2
    tests/smoke_test.cpp
)

target_link_libraries(test_gmDispatch_smoke_phase2 PRIVATE gmDispatch gmRules gmLog)
target_include_directories(test_gmDispatch_smoke_phase2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_executable(test_gmDispatch_smoke_phase3_4
    tests/smoke_test_phase3_4.cpp
)

target_link_libraries(test_gmDispatch_smoke_phase3_4 PRIVATE gmDispatch gmRules gmLog)
target_include_directories(test_gmDispatch_smoke_phase3_4 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME gmDispatch_smoke_phase2 COMMAND test_gmDispatch_smoke_phase2)
add_test(NAME gmDispatch_smoke_phase3_4 COMMAND test_gmDispatch_smoke_phase3_4)

add_custom_target(gmDispatch_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R gmDispatch_smoke_
    DEPENDS test_gmDispatch_smoke_phase2 test_gmDispatch_smoke_phase3_4
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "=== Running all gmDispatch tests ==="
    VERBATIM
)