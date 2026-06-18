/**
 * @file engine/DungeonEngine.cpp
 * @brief Stub implementation of DungeonEngine.
 *
 * All methods are placeholder stubs. Real logic will be introduced in FASE B.
 */

#include "engine/DungeonEngine.hpp"

#include <iostream>
#include <stdexcept>

namespace gmDungeonBasic
{

DungeonEngine::DungeonEngine()
	: _rules(_map, _actors)
	, _actions(_map, _actors, _rules)
{
	// ToBeImplemented //
}

void DungeonEngine::start_game(const std::string& map_file)
{
	(void)map_file;
	// ToBeImplemented //
	std::cout << "[DungeonEngine] start_game() called — stub.\n";
}

void DungeonEngine::handle_command(const std::string& typeId, const nlohmann::json& data)
{
	// ToBeImplemented //
	if (typeId == command_id::NEW_GAME)
	{
		start_game(data.value("map_file", ""));
	}
	else if (typeId == command_id::MOVE)
	{
		handle_move(data);
	}
	else if (typeId == command_id::HEAL)
	{
		handle_heal(data);
	}
	else if (typeId == command_id::EQUIP)
	{
		handle_equip(data);
	}
	else if (typeId == command_id::END_TURN)
	{
		handle_end_turn(data);
	}
	// Unknown commands are silently ignored.
}

void DungeonEngine::advance_turn()
{
	// ToBeImplemented //
}

// ── Private handlers ─────────────────────────────────────────────────────────

void DungeonEngine::handle_move(const nlohmann::json& data)
{
	(void)data;
	// ToBeImplemented //
}

void DungeonEngine::handle_heal(const nlohmann::json& data)
{
	(void)data;
	// ToBeImplemented //
}

void DungeonEngine::handle_equip(const nlohmann::json& data)
{
	(void)data;
	// ToBeImplemented //
}

void DungeonEngine::handle_end_turn(const nlohmann::json& data)
{
	(void)data;
	// ToBeImplemented //
}

// ── Snapshot builders ─────────────────────────────────────────────────────────

nlohmann::json DungeonEngine::build_map_snapshot() const
{
	// ToBeImplemented //
	return nlohmann::json::object();
}

nlohmann::json DungeonEngine::build_actor_snapshot() const
{
	// ToBeImplemented //
	return nlohmann::json::object();
}

} // namespace gmDungeonBasic
