/**
 * @file main.cpp
 * @brief Entry point for Le Pergamene di Eldhôm CoreEngine (P7 — GUI bridge).
 *
 * Loads the selected mission from GAME/Eldhom/data/, builds an EldhomEngine
 * and connects it to the PySide6 GUI via two TCP sockets:
 *
 * - Port 9210 (EVENTS):   engine connects to GUI as a client.  GUI is server.
 * - Port 9211 (COMMANDS): engine is the TCP server.  GUI connects as client.
 *
 * Wire format: 4-byte big-endian length prefix + UTF-8 JSON (identical to the
 * Dungeon Crawler Basic bridge, compatible with pyLib/gmGui/engine_bridge).
 *
 * Turn loop:
 * 1. Receive `eldhom.start_mission` command → load mission, emit full state.
 * 2. main loop:
 *    a. If next actor is HERO         → wait for GUI command.
 *    b. If next actor is MONSTER_GROUP → auto-resolve turn, emit events.
 *    c. If is_over()                  → emit victory/defeat, await new game.
 */

#include "GAME/Eldhom/CoreEngine/bridge/EldhomCmdServer.hpp"
#include "GAME/Eldhom/CoreEngine/bridge/EldhomGuiBridge.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomEngine.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionLoader.hpp"

#include "gmActor/core/Enums.hpp"
#include "gmSave/json.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// ─────────────────────────────────────────────────────────────────────────────
// Global shutdown flag
// ─────────────────────────────────────────────────────────────────────────────

namespace {
volatile std::sig_atomic_t g_running = 1;
void handle_signal(int) { g_running = 0; }
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// EldhomServer — owns engine + bridge and handles commands
// ─────────────────────────────────────────────────────────────────────────────

class EldhomServer
{
public:
	explicit EldhomServer(const std::string& data_dir)
		: _data_dir(data_dir)
		, _bridge("127.0.0.1", eldhom::ports::EVENTS)
	{
	}

	// Called by main loop for auto-advancing monster turns
	void advance_auto()
	{
		std::unique_lock<std::mutex> lock(_mtx);
		if (!_engine) { return; }
		if (_engine->is_over()) { return; }

		gmActor::ActorKind kind = _engine->next_actor_kind();
		if (kind == gmActor::ActorKind::MONSTER_GROUP)
		{
			std::string group_id = _engine->next_actor();
			_engine->resolve_group_turn_for(group_id);
			emit_next_actor_event();
			emit_full_state();
		}
	}

	// Dispatch a GUI command to the engine
	void handle_command(const std::string& type_id, const nlohmann::json& data)
	{
		std::unique_lock<std::mutex> lock(_mtx);

		// While a reaction window is open, only the defender's reaction (and a
		// state request) are accepted.  Every other command is rejected so the
		// engine stays the single source of truth for the pending attack.
		if (_engine && _engine->has_pending_attack()
		    && type_id != eldhom::CMD_REACT_DEFENSE
		    && type_id != eldhom::CMD_PLAY_INSTANTS
		    && type_id != eldhom::CMD_REQUEST_STATE)
		{
			nlohmann::json err;
			err["ok"]      = false;
			err["error"]   = "Finestra di reazione aperta: il difensore deve rispondere.";
			err["command"] = type_id;
			_bridge.send_event(eldhom::EVT_ACTION_RESULT, err);
			return;
		}

		if (type_id == eldhom::CMD_START_MISSION)
		{
			handle_start_mission(data);
		}
		else if (type_id == eldhom::CMD_PLAY_CARD)
		{
			handle_play_card(data);
		}
		else if (type_id == eldhom::CMD_SIMPLE_ACTION)
		{
			handle_simple_action(data);
		}
		else if (type_id == eldhom::CMD_DECLARE_ATTACK)
		{
			handle_declare_attack(data);
		}
		else if (type_id == eldhom::CMD_REACT_DEFENSE)
		{
			handle_react_defense(data);
		}
		else if (type_id == eldhom::CMD_PLAY_INSTANTS)
		{
			handle_play_instants(data);
		}
		else if (type_id == eldhom::CMD_STOP_SEQUENCE)
		{
			handle_stop_sequence(data);
		}
		else if (type_id == eldhom::CMD_REQUEST_STATE)
		{
			if (_engine) { emit_full_state(); }
		}
	}

private:
	// ── Mission start ──────────────────────────────────────────────────────────

	void handle_start_mission(const nlohmann::json& data)
	{
		std::string mission_id = data.value("mission_id", std::string{"missione_01"});

		try
		{
			std::cout << "[EldhomEngine] ✓ Received command: start_mission '" << mission_id << "'\n";
			eldhom::MissionDefinition def =
				eldhom::MissionLoader::load_mission(_data_dir, mission_id);

			std::unordered_map<eldhom::CardId, eldhom::EldhomCard> card_catalog =
				eldhom::MissionLoader::load_card_catalog(_data_dir);

			std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> behavior_catalog =
				eldhom::MissionLoader::load_behavior_catalogs_for_mission(def, _data_dir);

			_bridge_ref = &_bridge;
			_engine = std::make_unique<eldhom::EldhomEngine>(
				eldhom::EldhomEngine::from_definition(
					def,
					card_catalog,
					behavior_catalog,
					[this](const eldhom::EventType& type,
					       const std::string& actor_id,
					       const std::string& payload)
					{
						forward_engine_event(type, actor_id, payload);
					}));

			_last_def = def;

			emit_full_state();
			emit_next_actor_event();

			std::cout << "[EldhomEngine] Mission '" << mission_id << "' started.\n";
		}
		catch (const std::exception& ex)
		{
			nlohmann::json err;
			err["ok"]    = false;
			err["error"] = ex.what();
			_bridge.send_event(eldhom::EVT_ACTION_RESULT, err);
			std::cerr << "[EldhomEngine] load error: " << ex.what() << "\n";
		}
	}

	// ── PG commands ───────────────────────────────────────────────────────────

	void handle_play_card(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id = data.value("hero_id", std::string{});
		std::string card_id = data.value("card_id", std::string{});

		eldhom::ActionResult r = _engine->play_card(hero_id, card_id);
		send_action_result(r);
		if (r.ok()) { emit_next_actor_event(); emit_full_state(); }
	}

	void handle_simple_action(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id     = data.value("hero_id",     std::string{});
		std::string action_str  = data.value("action_type", std::string{});
		std::string destination = data.value("destination", std::string{});

		eldhom::SimpleActionType action = eldhom::SimpleActionType::RECOVER;
		if      (action_str == "MOVE")    { action = eldhom::SimpleActionType::MOVE; }
		else if (action_str == "ATTACK")  { action = eldhom::SimpleActionType::ATTACK; }
		else if (action_str == "INTERACT") { action = eldhom::SimpleActionType::INTERACT; }

		eldhom::ActionResult r = _engine->do_simple_action(hero_id, action, destination);
		send_action_result(r);
		if (r.ok()) { emit_next_actor_event(); emit_full_state(); }
	}

	// ── Interactive attack / reaction window commands ─────────────────────────

	void handle_declare_attack(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id   = data.value("hero_id",   std::string{});
		std::string target_id = data.value("target_id", std::string{});

		eldhom::ActionResult r = _engine->declare_attack(hero_id, target_id);
		send_action_result(r);
		if (!r.ok()) { return; }

		// Instants have priority: if any actor may answer with a matching
		// INSTANT card, open the instant window first; otherwise go straight
		// to the defender's reaction window.
		if (_engine->has_pending_instants())
		{
			emit_instant_window(_engine->pending_attack().instant_trigger);
		}
		else
		{
			emit_defense_window();
		}
	}

	// Builds and emits the instant-reaction window for the given trigger.
	void emit_instant_window(const std::string& trigger)
	{
		if (!_engine) { return; }

		nlohmann::json options = nlohmann::json::array();
		for (const eldhom::InstantOption& o : _engine->eligible_instants(trigger))
		{
			nlohmann::json oj;
			oj["actor_id"]  = o.actor_id;
			oj["card_id"]   = o.card_id;
			oj["card_name"] = o.card_name;
			options.push_back(oj);
		}

		nlohmann::json w;
		w["trigger"] = trigger;
		w["options"] = options;
		_bridge.send_event(eldhom::EVT_INSTANT_WINDOW_OPEN, w);
	}

	// Builds and emits the defender's reaction window from the pending attack.
	void emit_defense_window()
	{
		if (!_engine) { return; }
		const eldhom::PendingAttack& pa = _engine->pending_attack();

		nlohmann::json reactions = nlohmann::json::array();
		for (eldhom::DefenseReaction rk : _engine->allowed_reactions())
		{
			reactions.push_back(eldhom::to_string(rk));
		}

		std::string group_id;
		try
		{
			const gmActor::ActorStore& store = _engine->actor_store();
			if (store.has_actor(pa.defender_id) &&
			    store.actor_kind(pa.defender_id) == gmActor::ActorKind::MONSTER_INSTANCE)
			{
				group_id = store.monster_instance(pa.defender_id).group_id;
			}
		}
		catch (...) { /* group_id stays empty */ }

		nlohmann::json w;
		w["attacker_id"]     = pa.attacker_id;
		w["defender_id"]     = pa.defender_id;
		w["group_id"]        = group_id;
		w["incoming_damage"] = pa.base_damage;
		w["reactions"]       = reactions;
		_bridge.send_event(eldhom::EVT_REACTION_WINDOW_OPEN, w);
	}

	void handle_play_instants(const nlohmann::json& data)
	{
		if (!_engine) { return; }

		// Parse the selected (actor_id, card_id) pairs.
		std::vector<std::pair<std::string, std::string>> selected;
		if (data.contains("selected") && data["selected"].is_array())
		{
			for (const nlohmann::json& s : data["selected"])
			{
				std::string aid = s.value("actor_id", std::string{});
				std::string cid = s.value("card_id",  std::string{});
				if (!aid.empty() && !cid.empty())
				{
					selected.emplace_back(aid, cid);
				}
			}
		}

		eldhom::ActionResult r = _engine->play_instants(selected);
		send_action_result(r);
		if (!r.ok()) { return; }

		// Close the instant window, then open the defender's reaction window.
		nlohmann::json closed;
		closed["count"] = static_cast<int>(selected.size());
		_bridge.send_event(eldhom::EVT_INSTANT_WINDOW_CLOSED, closed);

		emit_full_state();
		emit_defense_window();
	}

	void handle_react_defense(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string defender_id   = data.value("defender_id", std::string{});
		std::string reaction_str  = data.value("reaction",    std::string{"TAKE"});

		eldhom::DefenseReaction reaction = eldhom::DefenseReaction::TAKE;
		if (!eldhom::parse_defense_reaction(reaction_str, reaction))
		{
			nlohmann::json err;
			err["ok"]    = false;
			err["error"] = "Unknown reaction: " + reaction_str;
			_bridge.send_event(eldhom::EVT_ACTION_RESULT, err);
			return;
		}

		eldhom::ReactionResolution res;
		eldhom::ActionResult r =
			_engine->resolve_reaction(defender_id, reaction, &res);
		send_action_result(r);
		if (!r.ok()) { return; }

		// Close the window and broadcast the resolution details.
		nlohmann::json closed;
		closed["defender_id"] = res.defender_id;
		_bridge.send_event(eldhom::EVT_REACTION_WINDOW_CLOSED, closed);

		nlohmann::json rj;
		rj["attacker_id"]       = res.attacker_id;
		rj["defender_id"]       = res.defender_id;
		rj["reaction"]          = eldhom::to_string(res.reaction);
		rj["base_damage"]       = res.base_damage;
		rj["final_damage"]      = res.final_damage;
		rj["defender_hp_after"] = res.defender_hp_after;
		rj["defender_ko"]       = res.defender_ko;
		_bridge.send_event(eldhom::EVT_ATTACK_RESOLVED, rj);

		emit_next_actor_event();
		emit_full_state();
	}

	void handle_stop_sequence(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id = data.value("hero_id", std::string{});

		eldhom::ActionResult r = _engine->stop_sequence(hero_id);
		send_action_result(r);
		if (r.ok()) { emit_next_actor_event(); emit_full_state(); }
	}

	// ── Event forwarding ──────────────────────────────────────────────────────

	void forward_engine_event(
		const eldhom::EventType& type,
		const std::string& actor_id,
		const std::string& payload)
	{
		std::cout << "[EldhomEngine] 📤 Forwarding event: " << type << "\n" << std::flush;
		nlohmann::json d;
		if (!actor_id.empty()) { d["actor_id"] = actor_id; }
		if (!payload.empty())
		{
			// payload may be a JSON array or plain string
			try
			{
				d["payload"] = nlohmann::json::parse(payload);
			}
			catch (...)
			{
				d["payload"] = payload;
			}
		}

		_bridge.send_event(type, d);

		// After victory/defeat, emit a summary
		if (type == eldhom::EVT_MISSION_VICTORY || type == eldhom::EVT_MISSION_DEFEAT)
		{
			std::cout << "[EldhomEngine] Mission ended: " << type << "\n";
		}
	}

	void send_action_result(const eldhom::ActionResult& r)
	{
		nlohmann::json d;
		d["ok"]    = r.ok();
		d["code"]  = static_cast<int>(r.code);
		d["error"] = r.message;
		_bridge.send_event(eldhom::EVT_ACTION_RESULT, d);
	}

	// ── State snapshot helpers ────────────────────────────────────────────────

	void emit_next_actor_event()
	{
		if (!_engine || _engine->is_over()) { return; }

		const std::string& id   = _engine->next_actor();
		gmActor::ActorKind  kind = _engine->next_actor_kind();

		std::string kind_str;
		switch (kind)
		{
		case gmActor::ActorKind::HERO:           kind_str = "HERO";          break;
		case gmActor::ActorKind::MONSTER_GROUP:  kind_str = "MONSTER_GROUP"; break;
		default:                                 kind_str = "OTHER";         break;
		}

		nlohmann::json d;
		d["actor_id"] = id;
		d["kind"]     = kind_str;
		_bridge.send_event(eldhom::EVT_TURN_NEXT_ACTOR, d);
	}

	void emit_full_state()
	{
		if (!_engine) { return; }

		std::cout << "[EldhomEngine] 📤 Building full state snapshot...\n" << std::flush;

		const gmActor::ActorStore& store = _engine->actor_store();
		nlohmann::json state;

		state["mission_id"] = _last_def.mission_id;
		state["title"]      = _last_def.title;
		state["time"]       = _engine->mission_time();

		// Locations
		nlohmann::json locs = nlohmann::json::array();
		for (const eldhom::LocationNode& loc : _last_def.locations)
		{
			nlohmann::json lj;
			lj["id"]   = loc.id;
			lj["name"] = loc.name;
			nlohmann::json adj = nlohmann::json::array();
			for (const std::string& a : loc.adjacent) { adj.push_back(a); }
			lj["adjacent"] = adj;
			locs.push_back(lj);
		}
		state["locations"] = locs;

		// Heroes
		nlohmann::json heroes = nlohmann::json::array();
		for (const auto& kv : store.heroes())
		{
			const gmActor::HeroState& h = kv.second;
			const gmActor::ActorStateCommon& c = h.common;
			nlohmann::json hj;
			hj["id"]       = c.actor_id;
			hj["name"]     = c.display_name;
			hj["faction"]  = c.faction_id;
			hj["location"] = c.area_id;
			hj["position"] =
				(c.area_position == gmActor::AreaPosition::FRONTLINE) ? "FRONTLINE" : "BACKLINE";
			hj["hp"]         = c.current_hp;
			hj["max_hp"]     = c.max_hp;
			hj["timeline"]   = c.timeline_position;
			hj["life_state"] = static_cast<int>(c.life_state);
			hj["hand_limit"] = h.hand_limit;

			// Hand cards
			try
			{
				nlohmann::json hand_arr = nlohmann::json::array();
				for (const eldhom::CardId& cid : _engine->hand_cards(c.actor_id))
				{
					hand_arr.push_back(cid);
				}
				hj["hand"]          = hand_arr;
				hj["deck_count"]    = _engine->deck_count(c.actor_id);
				hj["discard_count"] = _engine->discard_count(c.actor_id);
			}
			catch (...)
			{
				hj["hand"] = nlohmann::json::array();
				hj["deck_count"]    = 0;
				hj["discard_count"] = 0;
			}

			heroes.push_back(hj);
		}
		state["heroes"] = heroes;

		// Monster groups
		nlohmann::json groups = nlohmann::json::array();
		for (const auto& kv : store.monster_groups())
		{
			const gmActor::MonsterGroupState& g = kv.second;
			if (g.removed) { continue; }
			nlohmann::json gj;
			gj["id"]       = g.actor_id;
			gj["name"]     = g.display_name;
			gj["timeline"] = g.timeline_position;

			// First member's location
			std::string loc_id;
			if (!g.members.empty() && store.has_actor(g.members.front()))
			{
				loc_id = store.common(g.members.front()).area_id;
			}
			gj["location"] = loc_id;

			nlohmann::json insts = nlohmann::json::array();
			for (const gmActor::ActorId& mid : g.members)
			{
				if (!store.has_actor(mid)) { continue; }
				const gmActor::ActorStateCommon& mc = store.common(mid);
				nlohmann::json ij;
				ij["id"]       = mc.actor_id;
				ij["location"] = mc.area_id;
				ij["position"] =
					(mc.area_position == gmActor::AreaPosition::FRONTLINE) ? "FRONTLINE" : "BACKLINE";
				ij["hp"]     = mc.current_hp;
				ij["max_hp"] = mc.max_hp;
				ij["alive"]  = (mc.life_state == gmActor::ActorLifeState::ACTIVE);
				insts.push_back(ij);
			}
			gj["instances"] = insts;
			groups.push_back(gj);
		}
		state["groups"] = groups;

		// Next actor
		nlohmann::json nxt;
		nxt["actor_id"] = _engine->next_actor();
		nxt["kind"]     = [this]() -> std::string {
			switch (_engine->next_actor_kind())
			{
			case gmActor::ActorKind::HERO:          return "HERO";
			case gmActor::ActorKind::MONSTER_GROUP: return "MONSTER_GROUP";
			default:                                return "OTHER";
			}
		}();
		state["next_actor"] = nxt;
		state["is_over"]    = _engine->is_over();

		std::cout << "[EldhomEngine] 📤 Sending EVT_STATE_FULL to GUI with " 
		          << state["heroes"].size() << " heroes, " 
		          << state["groups"].size() << " groups\n" << std::flush;
		_bridge.send_event(eldhom::EVT_STATE_FULL, state);
		std::cout << "[EldhomEngine] ✓ EVT_STATE_FULL sent.\n" << std::flush;
	}

	// ── Data ──────────────────────────────────────────────────────────────────

	std::string                        _data_dir;
	eldhom::EldhomGuiBridge            _bridge;
	std::unique_ptr<eldhom::EldhomEngine> _engine;
	eldhom::MissionDefinition          _last_def;
	std::mutex                         _mtx;
	eldhom::EldhomGuiBridge*           _bridge_ref = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
	std::signal(SIGINT,  handle_signal);
	std::signal(SIGTERM, handle_signal);

	// data_dir: first arg or default relative to build location
	std::string data_dir = "GAME/Eldhom/data";
	if (argc >= 2) { data_dir = argv[1]; }

	std::cout << "[EldhomEngine] Le Pergamene di Eldhom — CoreEngine P7\n";
	std::cout << "[EldhomEngine] Data dir : " << data_dir << "\n";
	std::cout << "[EldhomEngine] Events   : connecting to GUI on port "
	          << eldhom::ports::EVENTS << "\n";
	std::cout << "[EldhomEngine] Commands : listening on port "
	          << eldhom::ports::COMMANDS << "\n";

	EldhomServer server(data_dir);

	eldhom::EldhomCmdServer cmd_server(
		eldhom::ports::COMMANDS,
		[&server](const std::string& type_id, const nlohmann::json& data)
		{
			try
			{
				server.handle_command(type_id, data);
			}
			catch (const std::exception& ex)
			{
				std::cerr << "[EldhomEngine] Command error: " << ex.what() << "\n";
			}
		});

	cmd_server.start();

	std::cout << "[EldhomEngine] Ready. Waiting for GUI connection on port 9211...\n";

	while (g_running)
	{
		server.advance_auto();
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	cmd_server.stop();
	std::cout << "[EldhomEngine] Stopped.\n";
	return 0;
}

