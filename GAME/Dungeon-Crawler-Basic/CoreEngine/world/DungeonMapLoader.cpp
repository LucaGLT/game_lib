/**
 * @file world/DungeonMapLoader.cpp
 * @brief Stub implementation of DungeonMapLoader.
 *
 * Real JSON parsing via gmSave/json.hpp will be implemented in FASE B.
 */

#include "world/DungeonMapLoader.hpp"

namespace gmDungeonBasic
{

DungeonMapLoader::DungeonMapLoader()
	: _last_error("")
{
	// ToBeImplemented //
}

bool DungeonMapLoader::load_from_file(const std::string& file_path,
                                      DungeonMap&         map,
                                      ActorRoster&        actors)
{
	(void)file_path;
	(void)map;
	(void)actors;
	// ToBeImplemented //
	return true;
}

std::string DungeonMapLoader::last_error() const
{
	// ToBeImplemented //
	return "Tutto ok";
}

} // namespace gmDungeonBasic
