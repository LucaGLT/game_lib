/**
 * @file engine/DungeonEngine.cpp
 * @brief Core orchestration for dungeon session commands/events.
 */

#include "engine/DungeonEngine.hpp"

#include <algorithm>
#include <filesystem>
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
	_actions.reset_combat();
	_pending_attack = PendingAttack{};
	_phase = GamePhase::HERO_TURN;
	_outcome = GameOutcome::NONE;

	std::string effective_map_file = map_file;
	if (effective_map_file.empty())
	{
		effective_map_file = ".cache/maps/dungeon_02.json";
	}

	if (!_loader.load_from_file(effective_map_file, _map, _actors, _room_meta))
	{
		throw std::runtime_error("Dungeon map load failed: " + _loader.last_error());
	}
	// Extract map_id from path basename for snapshot labelling.
	{
		const std::string base = std::filesystem::path(effective_map_file).stem().string();
		_current_map_id = base.empty() ? "dungeon" : base;
	}

	rebuild_interactables();

	const std::vector<std::string> heroes = _actors.heroes();
	const std::vector<std::string> enemies = _actors.enemies();
	std::vector<std::string> order;
	order.reserve(heroes.size() + enemies.size());
	order.insert(order.end(), heroes.begin(), heroes.end());
	order.insert(order.end(), enemies.begin(), enemies.end());

	_flow.start_session();
	_flow.set_actor_order(order);
	_actions_this_turn = 0;

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
			{{"actor_id", active_actor},
			 {"round", _flow.current_round()},
			 {"available_actions", build_available_actions(active_actor)},
			 {"actions_remaining", MAX_ACTIONS_PER_TURN}});
	}

	_log.log_session_start("s_001", effective_map_file);
}

void DungeonEngine::handle_command(const std::string& typeId, const nlohmann::json& data)
{
	// ToBeImplemented //
	if (typeId == command_id::NEW_GAME)
	{
		start_game(data.value("map_file", ""));
		return;
	}

	// While a reactive defense window is open the engine only accepts the
	// defender's response (or a view-only area query); everything else is held.
	if (_pending_attack.active
		&& typeId != command_id::DEFEND
		&& typeId != command_id::DEFEND_PASS
		&& typeId != command_id::AREA_INFO_REQUEST)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Finestra di difesa aperta: il difensore deve rispondere."},
			 {"command", typeId}});
		return;
	}

	if (typeId == command_id::MOVE)
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
	else if (typeId == command_id::PLAY_CARD)
	{
		handle_play_card(data);
	}
	else if (typeId == command_id::ATTACK)
	{
		handle_attack(data);
	}
	else if (typeId == command_id::DEFEND)
	{
		handle_defend(data);
	}
	else if (typeId == command_id::DEFEND_PASS)
	{
		handle_defend_pass(data);
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
	_actions_this_turn = 0;

	const bool is_hero = _actors.has_actor(next_actor) &&
	                     (_actors.get_actor(next_actor).kind == DungeonActorKind::HERO);
	const nlohmann::json avail = is_hero
		? build_available_actions(next_actor)
		: nlohmann::json::array();

	_gui.send_event(event_id::TURN_STARTED,
		{{"actor_id", next_actor},
		 {"round", _flow.current_round()},
		 {"available_actions", avail},
		 {"actions_remaining", MAX_ACTIONS_PER_TURN}});

	if (!is_hero)
	{
		// Mostri: turno automatico senza azioni (AI v1 = skip).
		_gui.send_event(event_id::TURN_ENDED, {{"actor_id", next_actor}});
		_flow.end_turn();
	}
}

// ── Private handlers ─────────────────────────────────────────────────────────

void DungeonEngine::handle_play_card(const nlohmann::json& data)
{
	// TODO: Phase 4 — Integra DungeonRuleAdapter::execute_card + eventi CARD_PLAYED.
	// ToBeImplemented //
	const std::string hero_id  = data.value("hero_id",  "hero");
	const std::string card_id  = data.value("card_id",  "");
	const std::string target_id = data.value("target_id", "");

	if (card_id.empty())
	{
		_gui.send_event(event_id::CARD_REJECTED,
			{{"hero_id",  hero_id},
			 {"card_id",  card_id},
			 {"reason",   "card_id mancante."}});
		return;
	}

	std::string rejection;
	const bool ok = _rules.execute_card(hero_id, card_id, target_id, rejection);
	if (!ok)
	{
		_gui.send_event(event_id::CARD_REJECTED,
			{{"hero_id", hero_id},
			 {"card_id", card_id},
			 {"reason",  rejection}});
		return;
	}

	_gui.send_event(event_id::CARD_PLAYED,
		{{"hero_id", hero_id},
		 {"card_id", card_id}});
}

void DungeonEngine::handle_attack(const nlohmann::json& data)
{
	if (_actions_this_turn >= MAX_ACTIONS_PER_TURN)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Azioni esaurite: premi Fine Turno per continuare."},
			 {"command", command_id::ATTACK}});
		return;
	}

	const std::string attacker_id = data.value("attacker_id", data.value("hero_id", ""));
	const std::string target_id   = data.value("target_id", "");
	const std::string card_id     = data.value("card_id", "");
	const int card_damage         = data.value("card_damage", 0);

	if (attacker_id.empty() || target_id.empty())
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Missing attacker_id or target_id."}, {"command", command_id::ATTACK}});
		return;
	}

	int base_damage = 0;
	if (!_actions.declare_attack(attacker_id, target_id, card_damage, base_damage))
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", _actions.last_rejection_reason()}, {"command", command_id::ATTACK}});
		_log.log_rejection(attacker_id, command_id::ATTACK, _actions.last_rejection_reason());
		return;
	}

	_pending_attack.attacker_id = attacker_id;
	_pending_attack.defender_id = target_id;
	_pending_attack.base_damage = base_damage;
	_pending_attack.source      = card_id.empty() ? std::string("base") : card_id;
	_pending_attack.active      = true;

	_gui.send_event(event_id::ATTACK_DECLARED,
		{{"attacker_id", attacker_id},
		 {"defender_id", target_id},
		 {"source",      _pending_attack.source},
		 {"base_damage", base_damage}});
	_gui.send_event(event_id::DEFENSE_WINDOW_OPENED,
		{{"defender_id",     target_id},
		 {"attacker_id",     attacker_id},
		 {"incoming_damage", base_damage},
		 {"can_pass",        true},
		 {"can_cancel",      true}});
	_log.log_action(attacker_id, command_id::ATTACK, target_id);
}

void DungeonEngine::handle_defend(const nlohmann::json& data)
{
	if (!_pending_attack.active)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Nessuna finestra di difesa aperta."}, {"command", command_id::DEFEND}});
		return;
	}

	const std::string defender_id = data.value("defender_id", _pending_attack.defender_id);
	if (defender_id != _pending_attack.defender_id)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Il difensore non corrisponde all'attacco in corso."},
			 {"command", command_id::DEFEND}});
		return;
	}

	const std::string mode = data.value("mode", "reduce");

	DefenseChoice choice;
	choice.cancel = (mode == "cancel");
	choice.pass   = false;
	choice.block  = data.value("block", 0);

	finalize_defense(choice);
}

void DungeonEngine::handle_defend_pass(const nlohmann::json& data)
{
	if (!_pending_attack.active)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Nessuna finestra di difesa aperta."},
			 {"command", command_id::DEFEND_PASS}});
		return;
	}

	const std::string defender_id = data.value("defender_id", _pending_attack.defender_id);
	if (defender_id != _pending_attack.defender_id)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Il difensore non corrisponde all'attacco in corso."},
			 {"command", command_id::DEFEND_PASS}});
		return;
	}

	DefenseChoice choice;
	choice.pass = true;
	finalize_defense(choice);
}

void DungeonEngine::finalize_defense(const DefenseChoice& defense)
{
	const std::string attacker_id = _pending_attack.attacker_id;
	const std::string defender_id = _pending_attack.defender_id;
	const int base_damage = _pending_attack.base_damage;

	const int before_hp = _actors.has_actor(defender_id)
		? _actors.get_actor(defender_id).hp
		: 0;

	int hp_after = before_hp;
	const int final_damage =
		_actions.resolve_attack(defender_id, base_damage, defense, hp_after);

	_gui.send_event(event_id::DEFENSE_WINDOW_CLOSED, {{"defender_id", defender_id}});
	_gui.send_event(event_id::ATTACK_RESOLVED,
		{{"attacker_id",  attacker_id},
		 {"defender_id",  defender_id},
		 {"base_damage",  base_damage},
		 {"final_damage", final_damage},
		 {"cancelled",    defense.cancel},
		 {"hp_after",     hp_after},
		 {"actions_remaining", MAX_ACTIONS_PER_TURN - _actions_this_turn - 1}});
	_gui.send_event(event_id::HP_CHANGED,
		{{"actor_id", defender_id}, {"delta", hp_after - before_hp}, {"hp_after", hp_after}});

	// Remove non-hero actors that have been reduced to 0 HP.
	if (hp_after <= 0 && _actors.has_actor(defender_id))
	{
		const ActorInfo fallen = _actors.get_actor(defender_id);
		if (fallen.kind != DungeonActorKind::HERO)
		{
			_actors.remove_actor(defender_id);
			_gui.send_event(event_id::ACTOR_REMOVED, {{"actor_id", defender_id}});
		}
	}

	_gui.send_event(event_id::ACTOR_SNAPSHOT, build_actor_snapshot());

	_log.log_action(attacker_id, command_id::ATTACK,
		defender_id + " dmg=" + std::to_string(final_damage));

	_pending_attack = PendingAttack{};
	++_actions_this_turn;
}

void DungeonEngine::handle_move(const nlohmann::json& data)
{
	if (_actions_this_turn >= MAX_ACTIONS_PER_TURN)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Azioni esaurite: premi Fine Turno per continuare."},
			 {"command", command_id::MOVE}});
		return;
	}
	const std::string hero_id = data.value("hero_id", "");
	const std::string destination = data.value("destination", "");
	const int max_distance = data.value("max_distance", 1);
	const std::string card_id = data.value("card_id", "");

	if (hero_id.empty() || destination.empty())
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Missing hero_id or destination."}, {"command", command_id::MOVE}});
		return;
	}

	const bool ok = (max_distance > 1)
		? _actions.execute_move(hero_id, destination, max_distance, card_id)
		: _actions.execute_move(hero_id, destination);

	if (!ok)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", _actions.last_rejection_reason()}, {"command", command_id::MOVE}});
		_log.log_rejection(hero_id, command_id::MOVE, _actions.last_rejection_reason());
		return;
	}

	std::string move_label = destination;
	if (!card_id.empty())
	{
		move_label += " (carta: " + card_id + ")";
	}
	std::cout << "[DungeonEngine] MOVE " << hero_id << " -> " << destination;
	if (!card_id.empty())
	{
		std::cout << " [card=" << card_id << ", max_dist=" << max_distance << "]";
	}
	std::cout << "\n";

	_gui.send_event(event_id::ACTOR_MOVED,
		{{"actor_id", hero_id}, {"to", destination}, {"actions_remaining", MAX_ACTIONS_PER_TURN - _actions_this_turn - 1}});
	_gui.send_event(event_id::ACTOR_SNAPSHOT, build_actor_snapshot());
	_log.log_action(hero_id, command_id::MOVE, move_label);
	++_actions_this_turn;
}

void DungeonEngine::handle_heal(const nlohmann::json& data)
{
	if (_actions_this_turn >= MAX_ACTIONS_PER_TURN)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Azioni esaurite: premi Fine Turno per continuare."},
			 {"command", command_id::HEAL}});
		return;
	}
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
	++_actions_this_turn;
}

void DungeonEngine::handle_equip(const nlohmann::json& data)
{
	if (_actions_this_turn >= MAX_ACTIONS_PER_TURN)
	{
		_gui.send_event(event_id::ACTION_REJECTED,
			{{"reason", "Azioni esaurite: premi Fine Turno per continuare."},
			 {"command", command_id::EQUIP}});
		return;
	}
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
	++_actions_this_turn;
}

void DungeonEngine::handle_end_turn(const nlohmann::json& data)
{
	const std::string hero_id = data.value("hero_id", _flow.current_actor_id());
	if (!_flow.is_turn_active())
	{
		return;
	}

	_gui.send_event(event_id::TURN_ENDED, {{"actor_id", hero_id}});
	_actions_this_turn = 0;
	_flow.end_turn();
}

nlohmann::json DungeonEngine::build_available_actions(const std::string& actor_id) const
{
	nlohmann::json actions = nlohmann::json::array();
	if (!_actors.has_actor(actor_id))
	{
		return actions;
	}
	const ActorInfo info = _actors.get_actor(actor_id);
	if (info.kind != DungeonActorKind::HERO)
	{
		return actions;
	}

	// Move è disponibile se la stanza corrente ha adiacenti.
	if (!_map.rooms_adjacent_to(info.location).empty())
	{
		actions.push_back("move");
	}
	if (_rules.can_heal(actor_id, actor_id))
	{
		actions.push_back("heal");
	}
	if (_rules.can_equip(actor_id, "bigword_available"))
	{
		actions.push_back("equip");
	}
	return actions;
}

// ── Snapshot builders ─────────────────────────────────────────────────────────

nlohmann::json DungeonEngine::build_map_snapshot() const
{
	nlohmann::json rooms = nlohmann::json::array();
	for (const std::string& room_id : _map.all_rooms())
	{
		nlohmann::json room;
		room["id"]       = room_id;
		room["tags"]     = _map.tags_of_room(room_id);
		room["adjacent"] = _map.rooms_adjacent_to(room_id);

		// Include zone / region / colour / items from loaded metadata.
		auto it = _room_meta.find(room_id);
		if (it != _room_meta.end())
		{
			const DungeonRoomMeta& m = it->second;
			room["zone_id"]            = m.zone_id;
			room["region_id"]          = m.region_id;
			room["zone_color_token"]   = m.zone_color_token;
			room["region_color_token"] = m.region_color_token;
			room["items"]              = m.items;
		}
		else
		{
			room["zone_id"]            = "";
			room["region_id"]          = "";
			room["zone_color_token"]   = "";
			room["region_color_token"] = "";
			room["items"]              = nlohmann::json::array();
		}
		rooms.push_back(room);
	}

	nlohmann::json out;
	out["map_id"] = _current_map_id.empty() ? "dungeon" : _current_map_id;
	out["rooms"]  = rooms;
	return out;
}

nlohmann::json DungeonEngine::build_actor_snapshot() const
{
	nlohmann::json actors = nlohmann::json::array();
	for (const std::string& actor_id : _actors.all_actor_ids())
	{
		const ActorInfo info = _actors.get_actor(actor_id);
		nlohmann::json row;
		row["id"]       = info.id;
		row["label"]    = info.label;
		row["kind"]     = actor_kind_to_string(info.kind);
		row["hp"]       = info.hp;
		row["max_hp"]   = info.max_hp;
		row["attack"]   = info.attack;
		row["defense"]  = info.defense;
		row["location"] = info.location;
		row["tags"]     = info.tags;
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
	if (_map.has_room(area_id))
	{
		// Presence is sourced from gmMap; display attributes from the roster.
		for (const std::string& actor_id : _map.actors_in_room(area_id))
		{
			if (!_actors.has_actor(actor_id))
			{
				continue;
			}
			const ActorInfo info = _actors.get_actor(actor_id);
			nlohmann::json row;
			row["id"]      = info.id;
			row["label"]   = info.label;
			row["name"]    = info.id;
			row["faction"] = actor_kind_to_string(info.kind);
			row["state"]   = info.statuses.empty() ? "ALIVE" : info.statuses.front();
			actors.push_back(row);
		}
	}

	nlohmann::json interactables = nlohmann::json::array();
	if (_map.has_room(area_id))
	{
		// Interactables are sourced natively from gmMap + gmInteraction.
		for (gmMap::InteractableObjectId obj_id : _map.interactables_in_room(area_id))
		{
			if (!_interactables.has(obj_id))
			{
				continue;
			}
			const gmInteraction::InteractableObject& obj = _interactables.get(obj_id);
			nlohmann::json entry;
			entry["id"] = obj.type;
			entry["name"] = obj.type;
			entry["type"] = "object";
			entry["state"] = gmInteraction::interaction_state_to_string(obj.state);
			interactables.push_back(entry);
		}
	}

	nlohmann::json out;
	out["area_id"] = area_id;
	out["actors"] = actors;
	out["interactables"] = interactables;
	return out;
}

void DungeonEngine::rebuild_interactables()
{
	_interactables.clear();
	_next_interactable_id = 1;

	for (const std::string& room_id : _map.all_rooms())
	{
		auto it = _room_meta.find(room_id);
		if (it == _room_meta.end())
			continue;
		for (const std::string& item_name : it->second.items)
		{
			const gmMap::InteractableObjectId obj_id = _next_interactable_id++;
			_interactables.create(obj_id, item_name, gmInteraction::InteractionState::IDLE);
			_map.place_interactable(room_id, obj_id);
		}
	}
}

} // namespace gmDungeonBasic
