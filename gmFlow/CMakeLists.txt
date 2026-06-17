# ── gmFlow – static library ─────────────────────────────────────────────────

add_library(gmFlow STATIC
    core/Result.cpp
    core/GameContext.cpp

    actors/ActorRegistry.cpp

    actions/ActionQueue.cpp
    actions/ActionWindow.cpp
    actions/StepBasedAction.cpp

    flow/Turn.cpp
    flow/Round.cpp
    flow/SequentialFlowController.cpp
    flow/TimelineFlowController.cpp
    flow/PhaseContext.cpp
    flow/FlowPhase.cpp
    bridges/FlowRulesGateway.cpp
    bridges/ActionGateway.cpp

    events/EventBus.cpp

    session/GameSession.cpp

    campaign/Campaign.cpp
)

target_include_directories(gmFlow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(gmFlow PUBLIC gmDispatch gmLog)

# ── Tests ────────────────────────────────────────────────────────────────────

add_executable(test_gmFlow_action_queue
    tests/test_action_queue.cpp
)
target_link_libraries(test_gmFlow_action_queue PRIVATE gmFlow gmDispatch gmLog)
target_include_directories(test_gmFlow_action_queue PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_executable(test_gmFlow_action_window
    tests/test_action_window.cpp
)
target_link_libraries(test_gmFlow_action_window PRIVATE gmFlow gmDispatch gmLog)
target_include_directories(test_gmFlow_action_window PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_executable(test_gmFlow_sequential
    tests/test_flow_sequential.cpp
)
target_link_libraries(test_gmFlow_sequential PRIVATE gmFlow gmDispatch gmLog)
target_include_directories(test_gmFlow_sequential PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_executable(test_gmFlow_campaign
    tests/test_flow_campaign.cpp
)
target_link_libraries(test_gmFlow_campaign PRIVATE gmFlow gmDispatch gmLog)
target_include_directories(test_gmFlow_campaign PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_executable(test_gmFlow_timeline
    tests/test_timeline_flow_controller.cpp
)
target_link_libraries(test_gmFlow_timeline PRIVATE gmFlow gmDispatch gmLog)
target_include_directories(test_gmFlow_timeline PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_executable(test_gmFlow_flow_phase
    tests/test_flow_phase.cpp
)
target_link_libraries(test_gmFlow_flow_phase PRIVATE gmFlow gmDispatch gmLog)
target_include_directories(test_gmFlow_flow_phase PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME gmFlow_action_queue  COMMAND test_gmFlow_action_queue)
add_test(NAME gmFlow_action_window COMMAND test_gmFlow_action_window)
add_test(NAME gmFlow_sequential    COMMAND test_gmFlow_sequential)
add_test(NAME gmFlow_campaign      COMMAND test_gmFlow_campaign)
add_test(NAME gmFlow_timeline      COMMAND test_gmFlow_timeline)
add_test(NAME gmFlow_flow_phase    COMMAND test_gmFlow_flow_phase)

add_custom_target(gmFlow_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R gmFlow_
    DEPENDS
        test_gmFlow_action_queue
        test_gmFlow_action_window
        test_gmFlow_sequential
        test_gmFlow_campaign
        test_gmFlow_timeline
        test_gmFlow_flow_phase
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "=== Running all gmFlow tests ==="
    VERBATIM
)
