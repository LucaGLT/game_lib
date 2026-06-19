/**
 * @file engine/DungeonEngine.cpp
 * @brief Core orchestration for dungeon session commands/events.
 */

#include "engine/DungeonEngine.hpp"

#include <algorithm>
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
	_map.reset();
	_actors.reset();
	_flow.reset();
	_phase = GamePhase::HERO_TURN;
	_outcome = GameOutcome::NONE;

	std::string effective_map_file = map_file;
	if (effective_map_file.empty())
	{
		effective_map_file = ".cache/maps/dungeon_01.json";
	}

	if (!_loader.load_from_file(effective_map_file, _map, _actors))
	{
		throw std::runtime_error("Dungeon map load failed: " + _loader.last_error());
	}

	const std::vector<std::string> heroes = _actors.heroes();
	const std::vector<std::string> enemies = _actors.enemies();
	std::vector<std::string> order;
	order.reserve(heroes.size() + enemies.size());
	order.insert(order.end(), heroes.begin(), heroes.end());
	order.insert(order.end(), enemies.begin(), enemies.end());

	_flow.start_session();
	_flow.set_actor_order(order);

	const std::string active_actor = _flow.next_actor_id();
	if (!active_actor.empty())
	{
		_flow.start_turn(active_actor);
	}

	_gui.send_event(event_id::SESSION_STARTED,
		{{"session_id", "s_001"}, {"round", _flow.current_round()}});
	_gui.send_event(event_id::MAP_SNAPSHOT, build_map_snapshot());
	_gui.send_event(event_id::ACTOR_SNAPSHOT, build_actor_snapshot());

	if (!active_actor.empty())
	{
		_gui.send_event(event_id::TURN_STARTED,
			{{"actor_id", active_actor}, {"round", _flow.current_round()}});
	}

	_log.log_session_start("s_001", effective_map_file);
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
	else if (typeId == command_id::AREA_INFO_REQUEST)
	{
		handle_area_info_request(data);
	}
	// AREA_SELECTED is a view-only hint with no engine side-effect; ignored here.
	// Unknown commands are silently ignored.
}

void DungeonEngine::advance_turn()
{
	if (!_flow.is_session_active() || _flow.is_turn_active())
	{
		return;
	}

	const std::string next_actor = _flow.next_actor_id();
	if (next_actor.empty())
	{
		return;
	}

	_flow.start_turn(next_actor);
	_gui.send_event(event_id::TURN_STARTED,
		{{"actor_id", next_actor}, {"round", _flow.current_round()}});
}

// ── Private handlers ─────────────────────────────────────────────────────────

void DungeonEngine::handle_move(const nlohmann::json& data)
{
	const std::string hero_id = data.value("hero_id", "");
	const std::string destination = data.value("destination", "");

	if (hero_id.empty() || destination.empty())
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Missing hero_id or destination."}, {"command", command_id::MOVE}});
		return;
	}

	if (!_actions.execute_move(hero_id, destination))
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", _actions.last_rejection_reason()}, {"command", command_id::MOVE}});
		_log.log_rejection(hero_id, command_id::MOVE, _actions.last_rejection_reason());
		return;
	}

	_gui.send_event(event_id::ACTOR_MOVED,
		{{"actor_id", hero_id}, {"to", destination}});
	_gui.send_event(event_id::ACTOR_SNAPSHOT, build_actor_snapshot());
	_log.log_action(hero_id, command_id::MOVE, destination);
}

void DungeonEngine::handle_heal(const nlohmann::json& data)
{
	const std::string hero_id = data.value("hero_id", "");
	const std::string target_id = data.value("target_id", hero_id);

	if (hero_id.empty())
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Missing hero_id."}, {"command", command_id::HEAL}});
		return;
	}

	const int before_hp = _actors.has_actor(target_id) ? _actors.get_actor(target_id).hp : 0;

	if (!_actions.execute_heal(hero_id, target_id))
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", _actions.last_rejection_reason()}, {"command", command_id::HEAL}});
		_log.log_rejection(hero_id, command_id::HEAL, _actions.last_rejection_reason());
		return;
	}

	const int after_hp = _actors.get_actor(target_id).hp;
	_gui.send_event(event_id::ACTOR_HEALED,
		{{"actor_id", hero_id}, {"target_id", target_id}, {"amount", 3}, {"hp_after", after_hp}});
	_gui.send_event(event_id::HP_CHANGED,
		{{"actor_id", target_id}, {"delta", after_hp - before_hp}, {"hp_after", after_hp}});
	_gui.send_event(event_id::ACTOR_SNAPSHOT, build_actor_snapshot());
	_log.log_action(hero_id, command_id::HEAL, target_id);
}

void DungeonEngine::handle_equip(const nlohmann::json& data)
{
	const std::string hero_id = data.value("hero_id", "");
	const std::string item_tag = data.value("item_tag", "");

	if (hero_id.empty() || item_tag.empty())
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Missing hero_id or item_tag."}, {"command", command_id::EQUIP}});
		return;
	}

	if (!_actions.execute_equip(hero_id, item_tag))
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", _actions.last_rejection_reason()}, {"command", command_id::EQUIP}});
		_log.log_rejection(hero_id, command_id::EQUIP, _actions.last_rejection_reason());
		return;
	}

	_gui.send_event(event_id::ACTOR_EQUIPPED,
		{{"actor_id", hero_id}, {"item_tag", item_tag}});
	_gui.send_event(event_id::ACTOR_SNAPSHOT, build_actor_snapshot());
	_log.log_action(hero_id, command_id::EQUIP, item_tag);
}

void DungeonEngine::handle_end_turn(const nlohmann::json& data)
{
	const std::string hero_id = data.value("hero_id", _flow.current_actor_id());
	if (!_flow.is_turn_active())
	{
		return;
	}

	_gui.send_event(event_id::TURN_ENDED, {{"actor_id", hero_id}});
	_flow.end_turn();
}

// ── Snapshot builders ─────────────────────────────────────────────────────────

nlohmann::json DungeonEngine::build_map_snapshot() const
{
	nlohmann::json rooms = nlohmann::json::array();
	for (const std::string& room_id : _map.all_rooms())
	{
		nlohmann::json room;
		room["id"] = room_id;
		room["tags"] = _map.tags_of_room(room_id);
		room["adjacent"] = _map.rooms_adjacent_to(room_id);
		rooms.push_back(room);
	}

	nlohmann::json out;
	out["map_id"] = "dungeon_01";
	out["rooms"] = rooms;
	return out;
}

nlohmann::json DungeonEngine::build_actor_snapshot() const
{
	nlohmann::json actors = nlohmann::json::array();
	for (const std::string& actor_id : _actors.all_actor_ids())
	{
		const ActorInfo info = _actors.get_actor(actor_id);
		nlohmann::json row;
		row["id"] = info.id;
		row["kind"] = actor_kind_to_string(info.kind);
		row["hp"] = info.hp;
		row["max_hp"] = info.max_hp;
		row["location"] = info.location;
		row["tags"] = info.tags;
		row["statuses"] = info.statuses;
		actors.push_back(row);
	}

	nlohmann::json out;
	out["actors"] = actors;
	return out;
}

void DungeonEngine::handle_area_info_request(const nlohmann::json& data)
{
	const std::string area_id = data.value("area_id", "");
	nlohmann::json response = build_area_info(area_id);
	if (data.contains("request_id"))
	{
		response["request_id"] = data.value("request_id", "");
	}
	_gui.send_event(event_id::AREA_INFO_RESPONSE, response);
}

nlohmann::json DungeonEngine::build_area_info(const std::string& area_id) const
{
	nlohmann::json actors = nlohmann::json::array();
	for (const std::string& actor_id : _actors.all_actor_ids())
	{
		const ActorInfo info = _actors.get_actor(actor_id);
		if (info.location != area_id)
		{
			continue;
		}
		nlohmann::json row;
		row["id"] = info.id;
		row["name"] = info.id;
		row["faction"] = actor_kind_to_string(info.kind);
		row["state"] = info.statuses.empty() ? "ALIVE" : info.statuses.front();
		actors.push_back(row);
	}

	nlohmann::json interactables = nlohmann::json::array();
	if (_map.has_room(area_id))
	{
		for (const std::string& tag : _map.tags_of_room(area_id))
		{
			nlohmann::json obj;
			obj["id"] = tag;
			obj["name"] = tag;
			obj["type"] = "tag";
			interactables.push_back(obj);
		}
	}

	nlohmann::json out;
	out["area_id"] = area_id;
	out["actors"] = actors;
	out["interactables"] = interactables;
	return out;
}

} // namespace gmDungeonBasic
