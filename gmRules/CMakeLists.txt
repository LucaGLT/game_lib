# ── gmRules – static library ───────────────────────────────────────────────────
#
# Public headers are under gmRules/. Consumers include them as:
#   #include "gmRules/facade/gmRulesEngine.hpp"
#
# gmRules intentionally does not depend on gmFlow.
# ────────────────────────────────────────────────────────────────────────────────

add_library(gmRules STATIC
    core/RuleResult.cpp
    core/RuleBook.cpp

    loader/RuleBookLoader.cpp

    target/TargetResult.cpp
    target/TargetResolver.cpp

    condition/ConditionEvaluator.cpp

    effect/EffectResult.cpp
    effect/EffectResolver.cpp

    status/StatusEngine.cpp

    facade/gmRulesEngine.cpp
)

target_include_directories(gmRules PUBLIC ${CMAKE_SOURCE_DIR})

# ── Shared test sources ────────────────────────────────────────────────────────

set(GMRULES_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/RuleResult.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/RuleBook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/loader/RuleBookLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/target/TargetResult.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/target/TargetResolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/condition/ConditionEvaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effect/EffectResult.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effect/EffectResolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/status/StatusEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/facade/gmRulesEngine.cpp
)

# ── Test executables ──────────────────────────────────────────────────────────

add_executable(test_gmRules_target_resolver
    ${GMRULES_COMMON_SOURCES}
    tests/test_target_resolver.cpp
)

add_executable(test_gmRules_condition_evaluator
    ${GMRULES_COMMON_SOURCES}
    tests/test_condition_evaluator.cpp
)

add_executable(test_gmRules_effect_resolver
    ${GMRULES_COMMON_SOURCES}
    tests/test_effect_resolver.cpp
)

add_executable(test_gmRules_status_engine
    ${GMRULES_COMMON_SOURCES}
    tests/test_status_engine.cpp
)

add_executable(test_gmRules_integration
    ${GMRULES_COMMON_SOURCES}
    tests/test_rules_integration.cpp
)

foreach(_t
    test_gmRules_target_resolver
    test_gmRules_condition_evaluator
    test_gmRules_effect_resolver
    test_gmRules_status_engine
    test_gmRules_integration
)
    target_include_directories(${_t} PRIVATE ${CMAKE_SOURCE_DIR})
endforeach()

# ── CTest registration ────────────────────────────────────────────────────────

add_test(NAME gmRules_target_resolver    COMMAND test_gmRules_target_resolver)
add_test(NAME gmRules_condition_evaluator COMMAND test_gmRules_condition_evaluator)
add_test(NAME gmRules_effect_resolver    COMMAND test_gmRules_effect_resolver)
add_test(NAME gmRules_status_engine      COMMAND test_gmRules_status_engine)
add_test(NAME gmRules_integration        COMMAND test_gmRules_integration)

# ── Convenience target ────────────────────────────────────────────────────────

add_custom_target(gmRules_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS
        test_gmRules_target_resolver
        test_gmRules_condition_evaluator
        test_gmRules_effect_resolver
        test_gmRules_status_engine
        test_gmRules_integration
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "=== Running all gmRules tests ==="
    VERBATIM
)
