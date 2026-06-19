/**
 * @file test_gmMap.cpp
 * @brief Test suite for gmMap (Region/Zone/Location model, snapshot v2).
 *
 * Uses gmLog JSON Lines output for test reporting.
 */

#include <algorithm>
#include <cstdint>
#include <functional>
#include <gmLog/LoggerFactory.hpp>
#include <gmMap/gmMap.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace
{

using TestMap = gmMap::gmMap<int>;

bool contains_id(const std::vector<gmMap::LocationId>& ids, gmMap::LocationId id)
{
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool contains_zone_id(const std::vector<gmMap::ZoneId>& ids, gmMap::ZoneId id)
{
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool contains_region_id(const std::vector<gmMap::RegionId>& ids, gmMap::RegionId id)
{
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool contains_actor(const std::vector<gmMap::ActorId>& ids, gmMap::ActorId id)
{
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool contains_object(const std::vector<gmMap::InteractableObjectId>& ids,
                     gmMap::InteractableObjectId id)
{
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

struct TestRunner
{
	gmLog::GmLogger logger;
	int passed;
	int failed;

	explicit TestRunner(const std::string& logPath)
	    : logger(gmLog::LoggerFactory::create_file_logger(
	          "gmMapTests", logPath, gmLog::LogLevel::DEBUG, true)),
	      passed(0), failed(0)
	{
	}

	void check(const std::string& name, bool ok)
	{
		if (ok)
		{
			++passed;
			logger.log(gmLog::LogLevel::INFO, "[PASS] " + name);
		}
		else
		{
			++failed;
			logger.log(gmLog::LogLevel::ERROR, "[FAIL] " + name);
		}
	}

	template <typename Exc, typename Fn> void check_throws(const std::string& name, Fn fn)
	{
		try
		{
			fn();
			++failed;
			logger.log(gmLog::LogLevel::ERROR, "[FAIL] " + name + " (expected exception)");
		}
		catch (const Exc&)
		{
			++passed;
			logger.log(gmLog::LogLevel::INFO, "[PASS] " + name);
		}
		catch (const std::exception& e)
		{
			++failed;
			logger.log(gmLog::LogLevel::ERROR,
			           "[FAIL] " + name + " (wrong exception: " + std::string(e.what()) + ")");
		}
	}

	int summary()
	{
		const int total = passed + failed;
		const std::string msg = "=== gmMap summary: " + std::to_string(passed) + "/" +
		                        std::to_string(total) + " passed";
		logger.log(gmLog::LogLevel::INFO, msg);
		std::cout << msg << std::endl;
		return failed == 0 ? 0 : 1;
	}
};

bool test_location_crud()
{
	TestMap m;

	m.create_location(1);
	m.create_location(2);
	m.create_location(3);

	if (m.location_count() != 3) return false;
	if (!m.has_location(1) || !m.has_location(2) || !m.has_location(3)) return false;

	std::vector<gmMap::LocationId> all = m.all_locations();
	if (all.size() != 3) return false;

	m.remove_location(2);
	if (m.location_count() != 2) return false;
	if (m.has_location(2)) return false;

	return true;
}

bool test_zone_crud()
{
	TestMap m;

	m.create_zone(10);
	m.create_zone(11);
	if (!m.has_zone(10) || !m.has_zone(11)) return false;
	if (m.zone_count() != 2) return false;

	std::vector<gmMap::ZoneId> all = m.all_zones();
	if (all.size() != 2) return false;

	m.remove_zone(10);
	if (m.has_zone(10)) return false;
	if (m.zone_count() != 1) return false;
	if (!contains_zone_id(m.all_zones(), 11)) return false;

	return true;
}

bool test_region_crud()
{
	TestMap m;

	m.create_region(100);
	m.create_region(101);
	if (!m.has_region(100) || !m.has_region(101)) return false;
	if (m.region_count() != 2) return false;

	std::vector<gmMap::RegionId> all = m.all_regions();
	if (all.size() != 2) return false;

	m.remove_region(100);
	if (m.has_region(100)) return false;
	if (m.region_count() != 1) return false;
	if (!contains_region_id(m.all_regions(), 101)) return false;

	return true;
}

bool test_zone_assignment_and_reassignment()
{
	TestMap m;

	m.create_location(100);
	m.create_location(101);
	m.create_zone(1);
	m.create_zone(2);

	m.assign_to_zone(100, 1);
	std::optional<gmMap::ZoneId> z1 = m.zone_of(100);
	if (!z1.has_value() || z1.value() != 1) return false;

	m.assign_to_zone(100, 2); // re-assignment
	std::optional<gmMap::ZoneId> z2 = m.zone_of(100);
	if (!z2.has_value() || z2.value() != 2) return false;

	std::vector<gmMap::LocationId> in1 = m.locations_in_zone(1);
	std::vector<gmMap::LocationId> in2 = m.locations_in_zone(2);

	if (contains_id(in1, 100)) return false;
	if (!contains_id(in2, 100)) return false;

	m.unassign_from_zone(100);
	std::optional<gmMap::ZoneId> z3 = m.zone_of(100);
	if (z3.has_value()) return false;

	// no-op expected
	m.unassign_from_zone(101);

	return true;
}

bool test_region_assignment_and_reassignment()
{
	TestMap m;

	m.create_zone(10);
	m.create_zone(11);
	m.create_region(1);
	m.create_region(2);

	m.assign_zone_to_region(10, 1);
	std::optional<gmMap::RegionId> r1 = m.region_of(10);
	if (!r1.has_value() || r1.value() != 1) return false;

	m.assign_zone_to_region(10, 2); // re-assignment
	std::optional<gmMap::RegionId> r2 = m.region_of(10);
	if (!r2.has_value() || r2.value() != 2) return false;

	std::vector<gmMap::ZoneId> in1 = m.zones_in_region(1);
	std::vector<gmMap::ZoneId> in2 = m.zones_in_region(2);

	if (contains_zone_id(in1, 10)) return false;
	if (!contains_zone_id(in2, 10)) return false;

	m.unassign_zone_from_region(10);
	std::optional<gmMap::RegionId> r3 = m.region_of(10);
	if (r3.has_value()) return false;

	// no-op expected
	m.unassign_zone_from_region(11);

	return true;
}

bool test_adjacency_directed_bidirectional()
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

	std::vector<gmMap::LocationId> adj = m.adjacent_to(1);
	if (!contains_id(adj, 3)) return false;

	m.remove_adjacent(1, 3, false);
	if (m.are_adjacent(1, 3)) return false;

	return true;
}

bool test_items()
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

bool test_actors()
{
	TestMap m;
	m.create_location(5);

	m.place_actor(5, 1001U);
	m.place_actor(5, 1002U);
	m.place_actor(5, 1001U); // idempotent

	std::vector<gmMap::ActorId> actors = m.actors_at(5);
	if (actors.size() != 2) return false;
	if (!contains_actor(actors, 1001U) || !contains_actor(actors, 1002U)) return false;
	if (!m.has_actor(5, 1001U)) return false;

	m.remove_actor(5, 1001U);
	if (m.has_actor(5, 1001U)) return false;
	if (m.actors_at(5).size() != 1) return false;

	// no-op expected
	m.remove_actor(5, 9999U);

	m.clear_actors(5);
	if (!m.actors_at(5).empty()) return false;

	return true;
}

bool test_interactables()
{
	TestMap m;
	m.create_location(6);

	m.place_interactable(6, 2001U);
	m.place_interactable(6, 2002U);
	m.place_interactable(6, 2001U); // idempotent

	std::vector<gmMap::InteractableObjectId> objs = m.interactables_at(6);
	if (objs.size() != 2) return false;
	if (!contains_object(objs, 2001U) || !contains_object(objs, 2002U)) return false;
	if (!m.has_interactable(6, 2001U)) return false;

	m.remove_interactable(6, 2001U);
	if (m.has_interactable(6, 2001U)) return false;
	if (m.interactables_at(6).size() != 1) return false;

	// no-op expected
	m.remove_interactable(6, 9999U);

	m.clear_interactables(6);
	if (!m.interactables_at(6).empty()) return false;

	return true;
}

bool test_location_metadata()
{
	TestMap m;
	m.create_location(50);

	m.set_location_meta(50, "name", std::string("Gate"));
	m.set_location_meta(50, "danger", static_cast<int64_t>(3));
	m.set_location_meta(50, "owner_uid", gmMap::UidRef{5001U});

	if (!m.has_location_meta(50, "name")) return false;
	if (!m.has_location_meta(50, "danger")) return false;
	if (!m.has_location_meta(50, "owner_uid")) return false;

	const gmMap::MetadataValue& n = m.get_location_meta(50, "name");
	const gmMap::MetadataValue& d = m.get_location_meta(50, "danger");
	const gmMap::MetadataValue& u = m.get_location_meta(50, "owner_uid");

	if (std::get<std::string>(n) != "Gate") return false;
	if (std::get<int64_t>(d) != 3) return false;
	if (std::get<gmMap::UidRef>(u).value != 5001U) return false;

	const gmMap::Metadata& meta = m.location_metadata(50);
	if (meta.size() != 3) return false;

	m.remove_location_meta(50, "danger");
	if (m.has_location_meta(50, "danger")) return false;

	// no-op expected
	m.remove_location_meta(50, "missing");

	return true;
}

bool test_zone_metadata()
{
	TestMap m;
	m.create_zone(99);

	m.set_zone_meta(99, "name", std::string("North"));
	m.set_zone_meta(99, "level", static_cast<int64_t>(2));
	m.set_zone_meta(99, "occupants", gmMap::UidList{gmMap::UidRef{77U}, gmMap::UidRef{88U}});

	if (!m.has_zone_meta(99, "name")) return false;
	if (!m.has_zone_meta(99, "level")) return false;
	if (!m.has_zone_meta(99, "occupants")) return false;

	const gmMap::MetadataValue& z = m.get_zone_meta(99, "name");
	const gmMap::MetadataValue& l = m.get_zone_meta(99, "level");
	const gmMap::MetadataValue& o = m.get_zone_meta(99, "occupants");

	if (std::get<std::string>(z) != "North") return false;
	if (std::get<int64_t>(l) != 2) return false;
	const gmMap::UidList& list = std::get<gmMap::UidList>(o);
	if (list.size() != 2U) return false;
	if (list[0].value != 77U || list[1].value != 88U) return false;

	const gmMap::Metadata& meta = m.zone_metadata(99);
	if (meta.size() != 3) return false;

	m.remove_zone_meta(99, "name");
	if (m.has_zone_meta(99, "name")) return false;

	// no-op expected
	m.remove_zone_meta(99, "missing");

	return true;
}

bool test_region_metadata()
{
	TestMap m;
	m.create_region(7);

	m.set_region_meta(7, "name", std::string("Continent"));
	m.set_region_meta(7, "tier", static_cast<int64_t>(1));

	if (!m.has_region_meta(7, "name")) return false;
	if (!m.has_region_meta(7, "tier")) return false;

	const gmMap::MetadataValue& n = m.get_region_meta(7, "name");
	if (std::get<std::string>(n) != "Continent") return false;

	const gmMap::Metadata& meta = m.region_metadata(7);
	if (meta.size() != 2) return false;

	m.remove_region_meta(7, "tier");
	if (m.has_region_meta(7, "tier")) return false;

	// no-op expected
	m.remove_region_meta(7, "missing");

	return true;
}

bool test_clear_full_reset()
{
	TestMap m;

	m.create_location(1);
	m.create_location(2);
	m.create_zone(5);
	m.create_region(50);
	m.assign_to_zone(1, 5);
	m.assign_zone_to_region(5, 50);
	m.set_adjacent(1, 2, true);
	m.add_item(1, 42);
	m.place_actor(1, 900U);
	m.place_interactable(1, 800U);
	m.set_location_meta(1, "k", 1);
	m.set_zone_meta(5, "t", 2);
	m.set_region_meta(50, "r", 3);

	m.clear();

	if (m.location_count() != 0) return false;
	if (m.zone_count() != 0) return false;
	if (m.region_count() != 0) return false;
	if (!m.all_locations().empty()) return false;
	if (!m.all_zones().empty()) return false;
	if (!m.all_regions().empty()) return false;

	return true;
}

bool test_remove_location_cascade()
{
	TestMap m;

	m.create_location(1);
	m.create_location(2);
	m.create_location(3);
	m.create_zone(9);

	m.assign_to_zone(1, 9);
	m.assign_to_zone(2, 9);

	m.set_adjacent(1, 2, true);  // 1<->2
	m.set_adjacent(3, 1, false); // 3->1

	m.remove_location(1);

	if (m.has_location(1)) return false;
	if (!m.has_location(2) || !m.has_location(3)) return false;

	std::vector<gmMap::LocationId> inZone = m.locations_in_zone(9);
	if (contains_id(inZone, 1)) return false;
	if (!contains_id(inZone, 2)) return false;

	std::vector<gmMap::LocationId> from2 = m.adjacent_to(2);
	std::vector<gmMap::LocationId> from3 = m.adjacent_to(3);
	if (contains_id(from2, 1)) return false;
	if (contains_id(from3, 1)) return false;

	return true;
}

bool test_remove_zone_cascade()
{
	TestMap m;

	m.create_location(10);
	m.create_location(11);
	m.create_zone(77);
	m.create_region(7);

	m.assign_to_zone(10, 77);
	m.assign_to_zone(11, 77);
	m.assign_zone_to_region(77, 7);

	m.remove_zone(77);

	if (m.has_zone(77)) return false;
	if (!m.has_location(10) || !m.has_location(11)) return false;

	std::optional<gmMap::ZoneId> z10 = m.zone_of(10);
	std::optional<gmMap::ZoneId> z11 = m.zone_of(11);
	if (z10.has_value()) return false;
	if (z11.has_value()) return false;

	// Zone must be detached from its region too
	if (contains_zone_id(m.zones_in_region(7), 77)) return false;

	return true;
}

bool test_remove_region_cascade()
{
	TestMap m;

	m.create_zone(20);
	m.create_zone(21);
	m.create_region(2);

	m.assign_zone_to_region(20, 2);
	m.assign_zone_to_region(21, 2);

	m.remove_region(2);

	if (m.has_region(2)) return false;
	if (!m.has_zone(20) || !m.has_zone(21)) return false;

	std::optional<gmMap::RegionId> r20 = m.region_of(20);
	std::optional<gmMap::RegionId> r21 = m.region_of(21);
	if (r20.has_value()) return false;
	if (r21.has_value()) return false;

	return true;
}

bool test_snapshot_roundtrip_v2()
{
	const std::string path = "test_gmMap_snapshot_v2.json";

	TestMap original;
	original.create_region(1);
	original.create_zone(10);
	original.create_zone(11);
	original.assign_zone_to_region(10, 1);

	original.create_location(100);
	original.create_location(101);
	original.create_location(102);
	original.assign_to_zone(100, 10);
	original.assign_to_zone(101, 10);
	original.assign_to_zone(102, 11);

	original.set_adjacent(100, 101, true);
	original.set_adjacent(101, 102, false);

	original.add_item(100, 7);
	original.add_item(100, 8);
	original.place_actor(100, 5000U);
	original.place_actor(100, 5001U);
	original.place_interactable(101, 6000U);

	original.set_location_meta(100, "name", std::string("Entrance"));
	original.set_zone_meta(10, "biome", std::string("cave"));
	original.set_region_meta(1, "world", std::string("Overworld"));

	original.export_snapshot_json(path);

	TestMap loaded;
	loaded.import_snapshot_json(path);

	// Structure
	if (loaded.region_count() != 1) return false;
	if (loaded.zone_count() != 2) return false;
	if (loaded.location_count() != 3) return false;

	// Hierarchy
	std::optional<gmMap::RegionId> r10 = loaded.region_of(10);
	if (!r10.has_value() || r10.value() != 1) return false;
	std::optional<gmMap::ZoneId> z100 = loaded.zone_of(100);
	if (!z100.has_value() || z100.value() != 10) return false;
	std::optional<gmMap::ZoneId> z102 = loaded.zone_of(102);
	if (!z102.has_value() || z102.value() != 11) return false;

	// Adjacency (directed edge preserved)
	if (!loaded.are_adjacent(100, 101)) return false;
	if (!loaded.are_adjacent(101, 100)) return false;
	if (!loaded.are_adjacent(101, 102)) return false;
	if (loaded.are_adjacent(102, 101)) return false;

	// Items
	const std::vector<int>& items100 = loaded.items_at(100);
	if (items100.size() != 2) return false;

	// Actors / interactables
	if (!loaded.has_actor(100, 5000U) || !loaded.has_actor(100, 5001U)) return false;
	if (loaded.actors_at(100).size() != 2) return false;
	if (!loaded.has_interactable(101, 6000U)) return false;

	// Metadata
	if (std::get<std::string>(loaded.get_location_meta(100, "name")) != "Entrance") return false;
	if (std::get<std::string>(loaded.get_zone_meta(10, "biome")) != "cave") return false;
	if (std::get<std::string>(loaded.get_region_meta(1, "world")) != "Overworld") return false;

	return true;
}

} // namespace

int main()
{
	TestRunner tr("test_gmMap_out.log");

	tr.check("T01 location CRUD + invariants", test_location_crud());
	tr.check_throws<gmMap::EDuplicateLocationError>("T02 duplicate location create throws",
	                                                []()
	                                                {
		                                                TestMap m;
		                                                m.create_location(5);
		                                                m.create_location(5);
	                                                });
	tr.check_throws<gmMap::EUnknownLocationError>("T03 remove missing location throws",
	                                              []()
	                                              {
		                                              TestMap m;
		                                              m.remove_location(404);
	                                              });

	tr.check("T04 zone CRUD + invariants", test_zone_crud());
	tr.check_throws<gmMap::EDuplicateZoneError>("T05 duplicate zone create throws",
	                                            []()
	                                            {
		                                            TestMap m;
		                                            m.create_zone(8);
		                                            m.create_zone(8);
	                                            });
	tr.check_throws<gmMap::EUnknownZoneError>("T06 remove missing zone throws",
	                                          []()
	                                          {
		                                          TestMap m;
		                                          m.remove_zone(808);
	                                          });

	tr.check("T07 region CRUD + invariants", test_region_crud());
	tr.check_throws<gmMap::EDuplicateRegionError>("T08 duplicate region create throws",
	                                              []()
	                                              {
		                                              TestMap m;
		                                              m.create_region(8);
		                                              m.create_region(8);
	                                              });
	tr.check_throws<gmMap::EUnknownRegionError>("T09 remove missing region throws",
	                                            []()
	                                            {
		                                            TestMap m;
		                                            m.remove_region(808);
	                                            });

	tr.check("T10 zone assignment + reassignment", test_zone_assignment_and_reassignment());
	tr.check_throws<gmMap::EUnknownLocationError>("T11 assign unknown location throws",
	                                              []()
	                                              {
		                                              TestMap m;
		                                              m.create_zone(1);
		                                              m.assign_to_zone(1, 1);
	                                              });
	tr.check_throws<gmMap::EUnknownZoneError>("T12 assign unknown zone throws",
	                                          []()
	                                          {
		                                          TestMap m;
		                                          m.create_location(1);
		                                          m.assign_to_zone(1, 1);
	                                          });

	tr.check("T13 region assignment + reassignment", test_region_assignment_and_reassignment());
	tr.check_throws<gmMap::EUnknownZoneError>("T14 assign unknown zone to region throws",
	                                          []()
	                                          {
		                                          TestMap m;
		                                          m.create_region(1);
		                                          m.assign_zone_to_region(1, 1);
	                                          });
	tr.check_throws<gmMap::EUnknownRegionError>("T15 assign zone to unknown region throws",
	                                            []()
	                                            {
		                                            TestMap m;
		                                            m.create_zone(1);
		                                            m.assign_zone_to_region(1, 1);
	                                            });

	tr.check("T16 adjacency directed/bidirectional", test_adjacency_directed_bidirectional());
	tr.check_throws<gmMap::EInvalidAdjacencyError>("T17 self-loop rejected",
	                                               []()
	                                               {
		                                               TestMap m;
		                                               m.create_location(1);
		                                               m.set_adjacent(1, 1, true);
	                                               });

	tr.check("T18 items add/remove/out-of-range base", test_items());
	tr.check_throws<gmMap::EInvalidItemIndexError>("T19 remove_item out-of-range throws",
	                                               []()
	                                               {
		                                               TestMap m;
		                                               m.create_location(1);
		                                               m.add_item(1, 7);
		                                               m.remove_item(1, 9);
	                                               });

	tr.check("T20 contained actors", test_actors());
	tr.check_throws<gmMap::EUnknownLocationError>("T21 place_actor unknown location throws",
	                                              []()
	                                              {
		                                              TestMap m;
		                                              m.place_actor(1, 1U);
	                                              });

	tr.check("T22 contained interactables", test_interactables());
	tr.check_throws<gmMap::EUnknownLocationError>("T23 place_interactable unknown location throws",
	                                              []()
	                                              {
		                                              TestMap m;
		                                              m.place_interactable(1, 1U);
	                                              });

	tr.check("T24 location metadata", test_location_metadata());
	tr.check_throws<gmMap::EUnknownMetaKeyError>("T25 missing location metadata key throws",
	                                             []()
	                                             {
		                                             TestMap m;
		                                             m.create_location(1);
		                                             (void)m.get_location_meta(1, "missing");
	                                             });

	tr.check("T26 zone metadata", test_zone_metadata());
	tr.check_throws<gmMap::EUnknownMetaKeyError>("T27 missing zone metadata key throws",
	                                             []()
	                                             {
		                                             TestMap m;
		                                             m.create_zone(1);
		                                             (void)m.get_zone_meta(1, "missing");
	                                             });

	tr.check("T28 region metadata", test_region_metadata());
	tr.check_throws<gmMap::EUnknownMetaKeyError>("T29 missing region metadata key throws",
	                                             []()
	                                             {
		                                             TestMap m;
		                                             m.create_region(1);
		                                             (void)m.get_region_meta(1, "missing");
	                                             });

	tr.check("T30 clear full reset", test_clear_full_reset());
	tr.check("T31 remove_location cascade", test_remove_location_cascade());
	tr.check("T32 remove_zone cascade", test_remove_zone_cascade());
	tr.check("T33 remove_region cascade", test_remove_region_cascade());
	tr.check("T34 snapshot v2 round-trip", test_snapshot_roundtrip_v2());

	return tr.summary();
}
