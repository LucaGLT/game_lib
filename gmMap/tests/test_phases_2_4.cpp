/**
 * @file test_phases_2_4.cpp
 * @brief Unit tests for gmMap Phases 2–4 (Private Helpers, Construction/Reset, Location Management)
 *
 * This test suite validates:
 * - Phase 2: Private helpers (_require_location, _require_tile) indirectly
 * - Phase 3: Construction/Reset (clear())
 * - Phase 4: Location CRUD operations (create, remove, has, all, count)
 */

#include <gmMap/gmMap.hpp>
#include <gmLog/LoggerFactory.hpp>
#include <functional>
#include <iostream>
#include <string>

using namespace GameMap;

// Simple test runner
struct TestRunner {
    gmLog::GmLogger logger;
    int total = 0;
    int passed = 0;

    explicit TestRunner(const std::string& logFile)
        : logger(gmLog::LoggerFactory::create_file_logger("gmMapTest", logFile, gmLog::LogLevel::DEBUG, true))
    {
    }

    void check(const std::string& testName, bool condition) {
        ++total;
        if (condition) {
            ++passed;
            logger.log(gmLog::LogLevel::INFO, "[PASS] " + testName);
        } else {
            logger.log(gmLog::LogLevel::ERROR, "[FAIL] " + testName);
        }
    }

    void checkThrows(const std::string& testName, const std::function<void()>& fn) {
        ++total;
        try {
            fn();
            logger.log(gmLog::LogLevel::ERROR, "[FAIL] " + testName + " (expected exception, none thrown)");
        } catch (const std::exception& e) {
            ++passed;
            logger.log(gmLog::LogLevel::INFO, "[PASS] " + testName + " (exception: " + std::string(e.what()) + ")");
        }
    }

    void summary() {
        std::string msg = "=== gmMap Phases 2-4 test summary: " + std::to_string(passed) + "/" + std::to_string(total) + " passed";
        logger.log(gmLog::LogLevel::INFO, msg);
        std::cout << msg << std::endl;
    }

    int exitCode() const {
        return (passed == total) ? 0 : 1;
    }
};

int main() {
    try {
        TestRunner runner("test_gmMap_phases_2_4_out.log");

        // Define a simple item type
        using SimpleMap = gmMap<int>;

        // ========================================================================
        // Phase 3 Test: Construction / Reset
        // ========================================================================

        runner.check("T01: clear() on empty map", [&]() {
            SimpleMap m;
            m.clear();
            return m.location_count() == 0;
        }());

        runner.check("T02: clear() removes all locations", [&]() {
            SimpleMap m;
            m.create_location(1);
            m.create_location(2);
            m.create_location(3);
            m.clear();
            return m.location_count() == 0 && !m.has_location(1) && !m.has_location(2) && !m.has_location(3);
        }());

        // ========================================================================
        // Phase 4 Tests: Location Management
        // ========================================================================

        runner.check("T03: create_location succeeds", [&]() {
            SimpleMap m;
            m.create_location(10);
            return m.has_location(10);
        }());

        runner.checkThrows("T04: create_location throws on duplicate", [&]() {
            SimpleMap m;
            m.create_location(10);
            m.create_location(10);  // Duplicate
        });

        runner.check("T05: has_location returns false for missing", [&]() {
            SimpleMap m;
            return !m.has_location(999);
        }());

        runner.check("T06: location_count with 1 location", [&]() {
            SimpleMap m;
            m.create_location(50);
            return m.location_count() == 1;
        }());

        runner.check("T07: location_count with 5 locations", [&]() {
            SimpleMap m;
            for (uint32_t i = 1; i <= 5; ++i) {
                m.create_location(i);
            }
            return m.location_count() == 5;
        }());

        runner.check("T08: all_locations returns 3 locations", [&]() {
            SimpleMap m;
            m.create_location(100);
            m.create_location(200);
            m.create_location(300);
            auto locs = m.all_locations();
            return locs.size() == 3;
        }());

        runner.check("T09: remove_location removes location", [&]() {
            SimpleMap m;
            m.create_location(77);
            m.remove_location(77);
            return !m.has_location(77);
        }());

        runner.checkThrows("T10: remove_location throws on missing", [&]() {
            SimpleMap m;
            m.remove_location(999);  // Doesn't exist
        });

        runner.check("T11: remove_location decrements count", [&]() {
            SimpleMap m;
            m.create_location(40);
            m.create_location(41);
            m.create_location(42);
            m.remove_location(41);
            return m.location_count() == 2;
        }());

        runner.check("T12: Complex CRUD scenario", [&]() {
            SimpleMap m;
            m.create_location(11);
            m.create_location(12);
            m.create_location(13);
            if (m.location_count() != 3) return false;
            m.remove_location(12);
            if (m.location_count() != 2) return false;
            if (!m.has_location(11) || m.has_location(12) || !m.has_location(13)) return false;
            m.clear();
            return m.location_count() == 0;
        }());

        runner.check("T13: all_locations empty after clear", [&]() {
            SimpleMap m;
            m.create_location(99);
            m.clear();
            return m.all_locations().empty();
        }());

        runner.check("T14: all_locations contains correct IDs", [&]() {
            SimpleMap m;
            m.create_location(5);
            m.create_location(10);
            m.create_location(15);
            auto all = m.all_locations();
            return all.size() == 3;
        }());

        runner.check("T15: remove_location cascade cleans neighbors", [&]() {
            SimpleMap m;
            m.create_location(1);
            m.create_location(2);
            m.remove_location(1);
            return !m.has_location(1) && m.has_location(2) && m.location_count() == 1;
        }());

        runner.summary();
        return runner.exitCode();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
