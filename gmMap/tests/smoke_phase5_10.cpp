#include <cstdint>
#include <gmMap/gmMap.hpp>
#include <optional>
#include <string>
#include <vector>

int main()
{
	gmMap::gmMap<int> map;

	map.create_location(1);
	map.create_location(2);
	map.create_tile(10);

	map.assign_to_tile(1, 10);
	map.unassign_from_tile(1);
	map.assign_to_tile(1, 10);

	std::optional<gmMap::TileId> t = map.tile_of(1);
	(void)t;

	std::vector<gmMap::LocationId> in_tile = map.locations_in_tile(10);
	(void)in_tile;

	map.set_adjacent(1, 2, true);
	bool adj = map.are_adjacent(1, 2);
	(void)adj;
	std::vector<gmMap::LocationId> neigh = map.adjacent_to(1);
	(void)neigh;
	map.remove_adjacent(1, 2, true);

	map.add_item(1, 99);
	const std::vector<int>& items = map.items_at(1);
	(void)items;
	map.remove_item(1, 0);
	map.clear_items(1);

	map.set_location_meta(1, "name", std::string("Bridge"));
	map.set_location_meta(1, "owner_uid", gmMap::UidRef{42U});
	bool has_lm = map.has_location_meta(1, "name");
	(void)has_lm;
	const gmMap::MetadataValue& lm = map.get_location_meta(1, "name");
	(void)lm;
	const gmMap::Metadata& lmeta = map.location_metadata(1);
	(void)lmeta;
	map.remove_location_meta(1, "name");

	map.set_tile_meta(10, "floor", static_cast<int64_t>(1));
	map.set_tile_meta(10, "uids", gmMap::UidList{gmMap::UidRef{10U}, gmMap::UidRef{11U}});
	bool has_tm = map.has_tile_meta(10, "floor");
	(void)has_tm;
	const gmMap::MetadataValue& tm = map.get_tile_meta(10, "floor");
	(void)tm;
	const gmMap::Metadata& tmeta = map.tile_metadata(10);
	(void)tmeta;
	map.remove_tile_meta(10, "floor");

	map.remove_tile(10);
	map.remove_location(2);
	map.remove_location(1);

	return 0;
}
