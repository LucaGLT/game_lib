#include "gmMap/gmMap.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>

/**
 * @brief Test snapshot export/import round-trip with full data recovery.
 */
bool test_snapshot_round_trip()
{
	// Create and populate a test map
	::gmMap::gmMap<int> m1;

	// Create locations
	m1.create_location(1);
	m1.create_location(2);
	m1.create_location(3);

	// Create zones
	m1.create_zone(100);
	m1.create_zone(101);

	// Assign locations to zones
	m1.assign_to_zone(1, 100);
	m1.assign_to_zone(2, 100);
	m1.assign_to_zone(3, 101);

	// Add adjacencies
	m1.set_adjacent(1, 2, /*bidirectional=*/true);
	m1.set_adjacent(2, 3, /*bidirectional=*/false);

	// Add items
	m1.add_item(1, 10);
	m1.add_item(1, 20);
	m1.add_item(2, 30);

	// Add location metadata
	m1.set_location_meta(1, "name", std::string("Location A"));
	m1.set_location_meta(1, "owner_id", ::gmMap::UidRef{1001U});
	m1.set_location_meta(2, "name", std::string("Location B"));
	m1.set_location_meta(3, "occupants", ::gmMap::UidList{{1001U}, {1002U}});

	// Add zone metadata
	m1.set_zone_meta(100, "zone", std::string("forest"));
	m1.set_zone_meta(100, "level", int64_t{1});
	m1.set_zone_meta(101, "zone", std::string("mountain"));

	// Export to JSON
	const std::string snapshot_path = "test_snapshot_temp.json";
	m1.export_snapshot_json(snapshot_path);

	// Verify file exists
	std::ifstream check_file(snapshot_path);
	if (!check_file.good())
	{
		std::cerr << "ERROR: Snapshot file was not created at " << snapshot_path << std::endl;
		return false;
	}
	check_file.close();

	// Create a new map and import
	::gmMap::gmMap<int> m2;
	m2.import_snapshot_json(snapshot_path);

	// Verify locations
	std::vector<::gmMap::LocationId> locs1 = m1.all_locations();
	std::vector<::gmMap::LocationId> locs2 = m2.all_locations();
	if (locs1.size() != locs2.size() || locs1.size() != 3)
	{
		std::cerr << "ERROR: Location count mismatch: " << locs1.size() << " vs " << locs2.size()
		          << std::endl;
		return false;
	}

	// Verify zones
	std::vector<::gmMap::ZoneId> zones1 = m1.all_zones();
	std::vector<::gmMap::ZoneId> zones2 = m2.all_zones();
	if (zones1.size() != zones2.size() || zones1.size() != 2)
	{
		std::cerr << "ERROR: Zone count mismatch" << std::endl;
		return false;
	}

	// Verify assignments
	for (::gmMap::LocationId loc : {1, 2, 3})
	{
		auto zone1 = m1.zone_of(loc);
		auto zone2 = m2.zone_of(loc);
		if (zone1.has_value() != zone2.has_value() ||
		    (zone1.has_value() && zone1.value() != zone2.value()))
		{
			std::cerr << "ERROR: Assignment mismatch for location " << loc << std::endl;
			return false;
		}
	}

	// Verify adjacencies
	if (!m2.are_adjacent(1, 2) || !m2.are_adjacent(2, 1))
	{
		std::cerr << "ERROR: Bidirectional adjacency 1-2 not restored" << std::endl;
		return false;
	}
	if (!m2.are_adjacent(2, 3))
	{
		std::cerr << "ERROR: Adjacency 2->3 not restored" << std::endl;
		return false;
	}
	if (m2.are_adjacent(3, 2))
	{
		std::cerr << "ERROR: Non-existent reverse adjacency 3->2 exists" << std::endl;
		return false;
	}

	// Verify items
	const auto& items1_loc1 = m1.items_at(1);
	const auto& items2_loc1 = m2.items_at(1);
	if (items1_loc1.size() != 2 || items2_loc1.size() != 2 || items1_loc1[0] != 10 ||
	    items2_loc1[0] != 10 || items1_loc1[1] != 20 || items2_loc1[1] != 20)
	{
		std::cerr << "ERROR: Items at location 1 mismatch" << std::endl;
		return false;
	}

	// Verify location metadata
	try
	{
		const auto& name1 = m1.get_location_meta(1, "name");
		const auto& name2 = m2.get_location_meta(1, "name");
		if (std::get<std::string>(name1) != std::get<std::string>(name2))
		{
			std::cerr << "ERROR: Location metadata 'name' mismatch" << std::endl;
			return false;
		}

		const auto& uid1 = m1.get_location_meta(1, "owner_id");
		const auto& uid2 = m2.get_location_meta(1, "owner_id");
		if (std::get<::gmMap::UidRef>(uid1).value != std::get<::gmMap::UidRef>(uid2).value)
		{
			std::cerr << "ERROR: Location metadata 'owner_id' UidRef mismatch" << std::endl;
			return false;
		}

		const auto& occupants1 = m1.get_location_meta(3, "occupants");
		const auto& occupants2 = m2.get_location_meta(3, "occupants");
		const auto& list1 = std::get<::gmMap::UidList>(occupants1);
		const auto& list2 = std::get<::gmMap::UidList>(occupants2);
		if (list1.size() != 2 || list2.size() != 2 || list1[0].value != list2[0].value ||
		    list1[1].value != list2[1].value)
		{
			std::cerr << "ERROR: Location metadata 'occupants' UidList mismatch" << std::endl;
			return false;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR: Exception accessing location metadata: " << e.what() << std::endl;
		return false;
	}

	// Verify zone metadata
	try
	{
		const auto& zone1 = m1.get_zone_meta(100, "zone");
		const auto& zone2 = m2.get_zone_meta(100, "zone");
		if (std::get<std::string>(zone1) != std::get<std::string>(zone2))
		{
			std::cerr << "ERROR: Zone metadata 'zone' mismatch" << std::endl;
			return false;
		}

		const auto& level1 = m1.get_zone_meta(100, "level");
		const auto& level2 = m2.get_zone_meta(100, "level");
		if (std::get<int64_t>(level1) != std::get<int64_t>(level2))
		{
			std::cerr << "ERROR: Zone metadata 'level' mismatch" << std::endl;
			return false;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR: Exception accessing zone metadata: " << e.what() << std::endl;
		return false;
	}

	// Clean up
	std::remove(snapshot_path.c_str());

	return true;
}

int main()
{
	std::cout << "\n=== gmMap Snapshot JSON Persistence Tests ===\n\n";

	bool pass = true;

	pass = test_snapshot_round_trip() && pass;
	std::cout << (pass ? "✓" : "✗") << " test_snapshot_round_trip\n";

	std::cout << "\n=== Summary: " << (pass ? "1/1 passed" : "0/1 failed") << " ===\n";
	return pass ? 0 : 1;
}
