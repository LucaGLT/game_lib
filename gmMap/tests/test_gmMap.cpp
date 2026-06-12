/**
 * @file test_gmMap.cpp
 * @brief Phase 11 test suite for gmMap.
 *
 * Uses gmLog JSON Lines output for test reporting.
 */

#include <gmMap/gmMap.hpp>
#include <gmLog/LoggerFactory.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace {

using TestMap = GameMap::gmMap<int>;

bool containsId(const std::vector<GameMap::LocationId>& ids, GameMap::LocationId id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool containsTileId(const std::vector<GameMap::TileId>& ids, GameMap::TileId id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

struct TestRunner {
    gmLog::GmLogger logger;
    int passed;
    int failed;

    explicit TestRunner(const std::string& logPath)
        : logger(gmLog::LoggerFactory::create_file_logger("gmMapPhase11", logPath, gmLog::LogLevel::DEBUG, true))
        , passed(0)
        , failed(0)
    {
    }

    void check(const std::string& name, bool ok)
    {
        if (ok) {
            ++passed;
            logger.log(gmLog::LogLevel::INFO, "[PASS] " + name);
        } else {
            ++failed;
            logger.log(gmLog::LogLevel::ERROR, "[FAIL] " + name);
        }
    }

    template <typename Exc, typename Fn>
    void checkThrows(const std::string& name, Fn fn)
    {
        try {
            fn();
            ++failed;
            logger.log(gmLog::LogLevel::ERROR, "[FAIL] " + name + " (expected exception)");
        } catch (const Exc&) {
            ++passed;
            logger.log(gmLog::LogLevel::INFO, "[PASS] " + name);
        } catch (const std::exception& e) {
            ++failed;
            logger.log(gmLog::LogLevel::ERROR, "[FAIL] " + name + " (wrong exception: " + std::string(e.what()) + ")");
        }
    }

    int summary()
    {
        const int total = passed + failed;
        const std::string msg = "=== gmMap Phase 11 summary: " + std::to_string(passed) + "/" + std::to_string(total) + " passed";
        logger.log(gmLog::LogLevel::INFO, msg);
        std::cout << msg << std::endl;
        return failed == 0 ? 0 : 1;
    }
};

bool testLocationCrud()
{
    TestMap m;

    m.create_location(1);
    m.create_location(2);
    m.create_location(3);

    if (m.location_count() != 3) return false;
    if (!m.has_location(1) || !m.has_location(2) || !m.has_location(3)) return false;

    std::vector<GameMap::LocationId> all = m.all_locations();
    if (all.size() != 3) return false;

    m.remove_location(2);
    if (m.location_count() != 2) return false;
    if (m.has_location(2)) return false;

    return true;
}

bool testTileCrud()
{
    TestMap m;

    m.create_tile(10);
    m.create_tile(11);
    if (!m.has_tile(10) || !m.has_tile(11)) return false;
    if (m.tile_count() != 2) return false;

    std::vector<GameMap::TileId> all = m.all_tiles();
    if (all.size() != 2) return false;

    m.remove_tile(10);
    if (m.has_tile(10)) return false;
    if (m.tile_count() != 1) return false;
    if (!containsTileId(m.all_tiles(), 11)) return false;

    return true;
}

bool testAssignmentAndReassignment()
{
    TestMap m;

    m.create_location(100);
    m.create_location(101);
    m.create_tile(1);
    m.create_tile(2);

    m.assign_to_tile(100, 1);
    std::optional<GameMap::TileId> t1 = m.tile_of(100);
    if (!t1.has_value() || t1.value() != 1) return false;

    m.assign_to_tile(100, 2); // re-assignment
    std::optional<GameMap::TileId> t2 = m.tile_of(100);
    if (!t2.has_value() || t2.value() != 2) return false;

    std::vector<GameMap::LocationId> in1 = m.locations_in_tile(1);
    std::vector<GameMap::LocationId> in2 = m.locations_in_tile(2);

    if (containsId(in1, 100)) return false;
    if (!containsId(in2, 100)) return false;

    m.unassign_from_tile(100);
    std::optional<GameMap::TileId> t3 = m.tile_of(100);
    if (t3.has_value()) return false;

    // no-op expected
    m.unassign_from_tile(101);

    return true;
}

bool testAdjacencyDirectedBidirectional()
{
    TestMap m;
    m.create_location(1);
    m.create_location(2);
    m.create_location(3);

    m.set_adjacent(1, 2, true);
    if (!m.are_adjacent(1, 2)) return false;
    if (!m.are_adjacent(2, 1)) return false;

    m.remove_adjacent(1, 2, true);
    if (m.are_adjacent(1, 2)) return false;
    if (m.are_adjacent(2, 1)) return false;

    m.set_adjacent(1, 3, false);
    if (!m.are_adjacent(1, 3)) return false;
    if (m.are_adjacent(3, 1)) return false;

    std::vector<GameMap::LocationId> adj = m.adjacent_to(1);
    if (!containsId(adj, 3)) return false;

    m.remove_adjacent(1, 3, false);
    if (m.are_adjacent(1, 3)) return false;

    return true;
}

bool testItems()
{
    TestMap m;
    m.create_location(7);

    m.add_item(7, 10);
    m.add_item(7, 20);
    m.add_item(7, 30);

    const std::vector<int>& before = m.items_at(7);
    if (before.size() != 3) return false;
    if (before[0] != 10 || before[1] != 20 || before[2] != 30) return false;

    m.remove_item(7, 1);
    const std::vector<int>& after = m.items_at(7);
    if (after.size() != 2) return false;
    if (after[0] != 10 || after[1] != 30) return false;

    m.clear_items(7);
    if (!m.items_at(7).empty()) return false;

    return true;
}

bool testLocationMetadata()
{
    TestMap m;
    m.create_location(50);

    m.set_location_meta(50, "name", std::string("Gate"));
    m.set_location_meta(50, "danger", static_cast<int64_t>(3));
    m.set_location_meta(50, "owner_uid", GameMap::UidRef{5001U});

    if (!m.has_location_meta(50, "name")) return false;
    if (!m.has_location_meta(50, "danger")) return false;
    if (!m.has_location_meta(50, "owner_uid")) return false;

    const GameMap::MetadataValue& n = m.get_location_meta(50, "name");
    const GameMap::MetadataValue& d = m.get_location_meta(50, "danger");
    const GameMap::MetadataValue& u = m.get_location_meta(50, "owner_uid");

    if (std::get<std::string>(n) != "Gate") return false;
    if (std::get<int64_t>(d) != 3) return false;
    if (std::get<GameMap::UidRef>(u).value != 5001U) return false;

    const GameMap::Metadata& meta = m.location_metadata(50);
    if (meta.size() != 3) return false;

    m.remove_location_meta(50, "danger");
    if (m.has_location_meta(50, "danger")) return false;

    // no-op expected
    m.remove_location_meta(50, "missing");

    return true;
}

bool testTileMetadata()
{
    TestMap m;
    m.create_tile(99);

    m.set_tile_meta(99, "zone", std::string("North"));
    m.set_tile_meta(99, "level", static_cast<int64_t>(2));
    m.set_tile_meta(99, "occupants", GameMap::UidList{GameMap::UidRef{77U}, GameMap::UidRef{88U}});

    if (!m.has_tile_meta(99, "zone")) return false;
    if (!m.has_tile_meta(99, "level")) return false;
    if (!m.has_tile_meta(99, "occupants")) return false;

    const GameMap::MetadataValue& z = m.get_tile_meta(99, "zone");
    const GameMap::MetadataValue& l = m.get_tile_meta(99, "level");
    const GameMap::MetadataValue& o = m.get_tile_meta(99, "occupants");

    if (std::get<std::string>(z) != "North") return false;
    if (std::get<int64_t>(l) != 2) return false;
    const GameMap::UidList& list = std::get<GameMap::UidList>(o);
    if (list.size() != 2U) return false;
    if (list[0].value != 77U || list[1].value != 88U) return false;

    const GameMap::Metadata& meta = m.tile_metadata(99);
    if (meta.size() != 3) return false;

    m.remove_tile_meta(99, "zone");
    if (m.has_tile_meta(99, "zone")) return false;

    // no-op expected
    m.remove_tile_meta(99, "missing");

    return true;
}

bool testClearFullReset()
{
    TestMap m;

    m.create_location(1);
    m.create_location(2);
    m.create_tile(5);
    m.assign_to_tile(1, 5);
    m.set_adjacent(1, 2, true);
    m.add_item(1, 42);
    m.set_location_meta(1, "k", 1);
    m.set_tile_meta(5, "t", 2);

    m.clear();

    if (m.location_count() != 0) return false;
    if (m.tile_count() != 0) return false;
    if (!m.all_locations().empty()) return false;
    if (!m.all_tiles().empty()) return false;

    return true;
}

bool testRemoveLocationCascade()
{
    TestMap m;

    m.create_location(1);
    m.create_location(2);
    m.create_location(3);
    m.create_tile(9);

    m.assign_to_tile(1, 9);
    m.assign_to_tile(2, 9);

    m.set_adjacent(1, 2, true);   // 1<->2
    m.set_adjacent(3, 1, false);  // 3->1

    m.remove_location(1);

    if (m.has_location(1)) return false;
    if (!m.has_location(2) || !m.has_location(3)) return false;

    std::vector<GameMap::LocationId> inTile = m.locations_in_tile(9);
    if (containsId(inTile, 1)) return false;
    if (!containsId(inTile, 2)) return false;

    std::vector<GameMap::LocationId> from2 = m.adjacent_to(2);
    std::vector<GameMap::LocationId> from3 = m.adjacent_to(3);
    if (containsId(from2, 1)) return false;
    if (containsId(from3, 1)) return false;

    return true;
}

bool testRemoveTileCascade()
{
    TestMap m;

    m.create_location(10);
    m.create_location(11);
    m.create_tile(77);

    m.assign_to_tile(10, 77);
    m.assign_to_tile(11, 77);

    m.remove_tile(77);

    if (m.has_tile(77)) return false;
    if (!m.has_location(10) || !m.has_location(11)) return false;

    std::optional<GameMap::TileId> t10 = m.tile_of(10);
    std::optional<GameMap::TileId> t11 = m.tile_of(11);

    if (t10.has_value()) return false;
    if (t11.has_value()) return false;

    return true;
}

} // namespace

int main()
{
    TestRunner tr("test_gmMap_out.log");

    tr.check("T01 location CRUD + invariants", testLocationCrud());
    tr.checkThrows<GameMap::DuplicateLocationError>("T02 duplicate location create throws", []() {
        TestMap m;
        m.create_location(5);
        m.create_location(5);
    });
    tr.checkThrows<GameMap::UnknownLocationError>("T03 remove missing location throws", []() {
        TestMap m;
        m.remove_location(404);
    });

    tr.check("T04 tile CRUD + invariants", testTileCrud());
    tr.checkThrows<GameMap::DuplicateTileError>("T05 duplicate tile create throws", []() {
        TestMap m;
        m.create_tile(8);
        m.create_tile(8);
    });
    tr.checkThrows<GameMap::UnknownTileError>("T06 remove missing tile throws", []() {
        TestMap m;
        m.remove_tile(808);
    });

    tr.check("T07 assignment + reassignment", testAssignmentAndReassignment());
    tr.checkThrows<GameMap::UnknownLocationError>("T08 assign unknown location throws", []() {
        TestMap m;
        m.create_tile(1);
        m.assign_to_tile(1, 1);
    });
    tr.checkThrows<GameMap::UnknownTileError>("T09 assign unknown tile throws", []() {
        TestMap m;
        m.create_location(1);
        m.assign_to_tile(1, 1);
    });

    tr.check("T10 adjacency directed/bidirectional", testAdjacencyDirectedBidirectional());
    tr.checkThrows<GameMap::InvalidAdjacencyError>("T11 self-loop rejected", []() {
        TestMap m;
        m.create_location(1);
        m.set_adjacent(1, 1, true);
    });

    tr.check("T12 items add/remove/out-of-range base", testItems());
    tr.checkThrows<GameMap::InvalidItemIndexError>("T13 remove_item out-of-range throws", []() {
        TestMap m;
        m.create_location(1);
        m.add_item(1, 7);
        m.remove_item(1, 9);
    });

    tr.check("T14 location metadata", testLocationMetadata());
    tr.checkThrows<GameMap::UnknownMetaKeyError>("T15 missing location metadata key throws", []() {
        TestMap m;
        m.create_location(1);
        (void)m.get_location_meta(1, "missing");
    });

    tr.check("T16 tile metadata", testTileMetadata());
    tr.checkThrows<GameMap::UnknownMetaKeyError>("T17 missing tile metadata key throws", []() {
        TestMap m;
        m.create_tile(1);
        (void)m.get_tile_meta(1, "missing");
    });

    tr.check("T18 clear full reset", testClearFullReset());
    tr.check("T19 remove_location cascade", testRemoveLocationCascade());
    tr.check("T20 remove_tile cascade", testRemoveTileCascade());

    return tr.summary();
}
