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

/// @brief Auto-dismiss timeout for a monster-action popup left unacknowledged
/// (no client clicked it). Enforced by EldhomServer::advance_auto()'s
/// existing 200ms polling loop — no dedicated timer/thread needed.
constexpr std::chrono::milliseconds MONSTER_POPUP_TIMEOUT{3000};

/// @brief Parses "--flag_name <value>" from argv, or returns fallback if absent/invalid.
uint16_t read_port_option(int argc, char** argv, const std::string& flag_name, uint16_t fallback)
{
	for (int i = 1; i < argc - 1; ++i)
	{
		if (flag_name == argv[i])
		{
			try
			{
				return static_cast<uint16_t>(std::stoi(argv[i + 1]));
			}
			catch (const std::exception&)
			{
				return fallback;
			}
		}
	}
	return fallback;
}
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// EldhomServer — owns engine + bridge and handles commands
// ─────────────────────────────────────────────────────────────────────────────

class EldhomServer
{
public:
	explicit EldhomServer(const std::string& data_dir, uint16_t events_port = eldhom::ports::EVENTS)
		: _data_dir(data_dir)
		, _bridge("127.0.0.1", events_port)
	{
	}

	// Called by main loop for auto-advancing monster turns
	void advance_auto()
	{
		std::unique_lock<std::mutex> lock(_mtx);
		if (!_engine) { return; }
		if (_engine->is_over()) { return; }

		// While a monster-action popup is shown, no new group turn starts —
		// only the 3s auto-dismiss timeout may advance it here. An explicit
		// client ack is handled separately by handle_ack_monster_popup().
		if (_engine->has_pending_monster_popup())
		{
			auto elapsed = std::chrono::steady_clock::now() - _monster_popup_shown_at;
			if (elapsed >= MONSTER_POPUP_TIMEOUT)
			{
				advance_monster_popup();
			}
			return;
		}

		if (_engine->has_pending_reactive_window()) { return; } // await GUI response

		gmActor::ActorKind kind = _engine->next_actor_kind();
		if (kind == gmActor::ActorKind::MONSTER_GROUP)
		{
			std::string group_id = _engine->next_actor();
			_engine->resolve_group_turn_for(group_id);
			if (_engine->has_pending_monster_popup())
			{
				emit_monster_popup();
				return; // pause here; resume once acknowledged or timed out
			}
			if (_engine->has_pending_reactive_window())
			{
				emit_instant_window(_engine->pending_reactive_window().trigger);
			}
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

		// While a formation dialog is open, only the resolve command and state
		// requests are accepted.
		if (_engine && _engine->has_pending_formation()
		    && type_id != eldhom::CMD_RESOLVE_FORMATION
		    && type_id != eldhom::CMD_REQUEST_STATE)
		{
			nlohmann::json err;
			err["ok"]      = false;
			err["error"]   = "Finestra di formazione aperta: risolvi prima la formazione.";
			err["command"] = type_id;
			_bridge.send_event(eldhom::EVT_ACTION_RESULT, err);
			return;
		}

		// While a reactive instant window (Assestarsi — enemy approach) is open,
		// only the matching command and state requests are accepted.
		if (_engine && _engine->has_pending_reactive_window()
		    && type_id != eldhom::CMD_PLAY_REACTIVE_INSTANTS
		    && type_id != eldhom::CMD_REQUEST_STATE)
		{
			nlohmann::json err;
			err["ok"]      = false;
			err["error"]   = "Finestra reattiva aperta: rispondi prima con eldhom.play_reactive_instants.";
			err["command"] = type_id;
			_bridge.send_event(eldhom::EVT_ACTION_RESULT, err);
			return;
		}

		// While a hero's end-of-turn confirmation is pending (they already
		// completed their one allowed action/card/sequence this turn), only
		// their own Fine Turno confirmation (a simple_action command — validated
		// as PASS by the engine itself) and state requests are accepted. Richiesta
		// esplicita utente: il turno non deve MAI passare al prossimo Attore senza
		// una conferma esplicita, anche quando il PG non ha più nulla da fare.
		if (_engine && _engine->has_pending_turn_confirmation()
		    && type_id != eldhom::CMD_SIMPLE_ACTION
		    && type_id != eldhom::CMD_REQUEST_STATE)
		{
			nlohmann::json err;
			err["ok"]      = false;
			err["error"]   = "Conferma Fine Turno in sospeso: premi Fine Turno prima di continuare.";
			err["command"] = type_id;
			_bridge.send_event(eldhom::EVT_ACTION_RESULT, err);
			return;
		}

		// While a monster-action popup is shown (see advance_auto()), only its
		// ack command and state requests are accepted — any connected client
		// may dismiss it, closing it for everyone. Richiesta esplicita utente:
		// non deve avvenire l'azione successiva di un Mostro finché il pop-up
		// dell'azione precedente non si è chiuso.
		if (_engine && _engine->has_pending_monster_popup()
		    && type_id != eldhom::CMD_ACK_MONSTER_POPUP
		    && type_id != eldhom::CMD_REQUEST_STATE)
		{
			nlohmann::json err;
			err["ok"]      = false;
			err["error"]   = "Pop-up azione mostro aperto: attendi la chiusura prima di continuare.";
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
		else if (type_id == eldhom::CMD_PLAY_REACTIVE_INSTANTS)
		{
			handle_play_reactive_instants(data);
		}
		else if (type_id == eldhom::CMD_RESOLVE_FORMATION)
		{
			handle_resolve_formation(data);
		}
		else if (type_id == eldhom::CMD_ACK_MONSTER_POPUP)
		{
			handle_ack_monster_popup(data);
		}
		else if (type_id == eldhom::CMD_STOP_SEQUENCE)
		{
			handle_stop_sequence(data);
		}
		else if (type_id == eldhom::CMD_DECK_DRAW)
		{
			handle_deck_draw(data);
		}
		else if (type_id == eldhom::CMD_DECK_DISCARD)
		{
			handle_deck_discard(data);
		}
		else if (type_id == eldhom::CMD_DECK_TAKE_DISCARD)
		{
			handle_deck_take_discard(data);
		}
		else if (type_id == eldhom::CMD_DECK_RESHUFFLE)
		{
			handle_deck_reshuffle(data);
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
		std::string hero_id     = data.value("hero_id",     std::string{});
		std::string card_id     = data.value("card_id",     std::string{});
		std::string destination = data.value("destination", std::string{});
		std::string target_id   = data.value("target_id",   std::string{});

		std::vector<std::string> discard_ids;
		if (data.contains("discard_ids") && data["discard_ids"].is_array())
		{
			for (const nlohmann::json& cid : data["discard_ids"])
			{
				discard_ids.push_back(cid.get<std::string>());
			}
		}

		eldhom::ActionResult r =
			_engine->play_card(hero_id, card_id, destination, discard_ids, target_id);
		send_action_result(r);
		if (!r.ok()) { return; }

		// If the card parked a DAMAGE pending attack, open the reaction chain
		// (instant window first if eligible, then defense window).  Otherwise
		// the card had no DAMAGE effect and the turn advances normally.
		if (_engine->has_pending_attack())
		{
			if (_engine->has_pending_instants())
			{
				emit_instant_window(_engine->pending_attack().instant_trigger);
			}
			else
			{
				emit_defense_window();
			}
		}
		else if (_engine->has_pending_formation())
		{
			emit_formation_dialog();
		}
		else
		{
			emit_next_actor_event();
			emit_full_state();
		}
	}

	void handle_simple_action(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id     = data.value("hero_id",     std::string{});
		std::string action_str  = data.value("action_type", std::string{});
		std::string destination = data.value("destination", std::string{});

		std::vector<std::string> discard_ids;
		if (data.contains("discard_ids") && data["discard_ids"].is_array())
		{
			for (const nlohmann::json& cid : data["discard_ids"])
			{
				discard_ids.push_back(cid.get<std::string>());
			}
		}

		eldhom::SimpleActionType action = eldhom::SimpleActionType::RECOVER;
		if      (action_str == "MOVE")     { action = eldhom::SimpleActionType::MOVE; }
		else if (action_str == "ATTACK")   { action = eldhom::SimpleActionType::ATTACK; }
		else if (action_str == "INTERACT") { action = eldhom::SimpleActionType::INTERACT; }
		else if (action_str == "PASS")     { action = eldhom::SimpleActionType::PASS; }

		eldhom::ActionResult r =
			_engine->do_simple_action(hero_id, action, destination, discard_ids);
		send_action_result(r);
		if (!r.ok()) { return; }

		if (_engine->has_pending_formation())
		{
			emit_formation_dialog();
		}
		else
		{
			emit_next_actor_event();
			emit_full_state();
		}
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

	// Emits the currently shown monster-action popup and records the wall-
	// clock timestamp used by advance_auto()'s 3-second auto-dismiss check.
	void emit_monster_popup()
	{
		if (!_engine) { return; }
		const eldhom::PendingMonsterPopup& p = _engine->current_monster_popup();

		nlohmann::json d;
		d["actor_id"]    = p.actor_id;
		d["description"] = p.description;
		_monster_popup_shown_at = std::chrono::steady_clock::now();
		_bridge.send_event(eldhom::EVT_MONSTER_ACTION_POPUP, d);
	}

	/**
	 * @brief Acknowledges the current monster-action popup (explicit ack or
	 * timeout) and either shows the next queued one or, once the queue is
	 * empty, resumes the normal turn-advance flow.
	 */
	void advance_monster_popup()
	{
		if (!_engine) { return; }
		_engine->acknowledge_monster_popup();

		if (_engine->has_pending_monster_popup())
		{
			emit_monster_popup();
			return;
		}

		_bridge.send_event(eldhom::EVT_MONSTER_POPUP_CLOSED, nlohmann::json::object());
		if (_engine->has_pending_reactive_window())
		{
			emit_instant_window(_engine->pending_reactive_window().trigger);
		}
		emit_next_actor_event();
		emit_full_state();
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

	// Builds and emits the formation dialog for the front-of-queue item.
	void emit_formation_dialog()
	{
		if (!_engine || !_engine->has_pending_formation()) { return; }

		const eldhom::PendingFormation& pf = _engine->current_formation_dialog();

		nlohmann::json actors_arr = nlohmann::json::array();
		for (const eldhom::ActorFormationEntry& e : pf.actors)
		{
			nlohmann::json a;
			a["actor_id"]    = e.actor_id;
			a["name"]        = e.display_name;
			a["in_backline"] = e.in_backline;
			actors_arr.push_back(a);
		}

		nlohmann::json w;
		w["location_id"] = pf.location_id;
		w["faction_id"]  = pf.faction_id;
		w["source"]      = pf.source;
		w["actors"]      = actors_arr;
		_bridge.send_event(eldhom::EVT_FORMATION_DIALOG, w);
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

	// Handles the GUI's response to a reactive instant window (Assestarsi —
	// enemy approach). Unlike CMD_PLAY_INSTANTS this never opens a defense
	// window afterwards: there is no attack to resolve, so the engine simply
	// closes the window and lets auto-advance continue.
	void handle_play_reactive_instants(const nlohmann::json& data)
	{
		if (!_engine) { return; }

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

		eldhom::ActionResult r = _engine->play_reactive_instants(selected);
		send_action_result(r);
		if (!r.ok()) { return; }

		emit_next_actor_event();
		emit_full_state();
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

		if (_engine->has_pending_formation())
		{
			emit_formation_dialog();
		}
		else
		{
			emit_next_actor_event();
			emit_full_state();
		}
	}

	void handle_stop_sequence(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id = data.value("hero_id", std::string{});

		eldhom::ActionResult r = _engine->stop_sequence(hero_id);
		send_action_result(r);
		if (r.ok()) { emit_next_actor_event(); emit_full_state(); }
	}

	void handle_resolve_formation(const nlohmann::json& data)
	{
		if (!_engine) { return; }

		std::string faction_id  = data.value("faction_id",  std::string{});
		std::string location_id = data.value("location_id", std::string{});

		std::vector<std::string> backline_ids;
		if (data.contains("backline") && data["backline"].is_array())
		{
			for (const nlohmann::json& id : data["backline"])
			{
				if (id.is_string()) { backline_ids.push_back(id.get<std::string>()); }
			}
		}

		eldhom::ActionResult r =
			_engine->resolve_formation(faction_id, location_id, backline_ids);
		send_action_result(r);
		if (!r.ok()) { return; }

		nlohmann::json done;
		done["faction_id"]  = faction_id;
		done["location_id"] = location_id;
		_bridge.send_event(eldhom::EVT_FORMATION_DONE, done);

		if (_engine->has_pending_formation())
		{
			emit_formation_dialog();
		}
		else
		{
			emit_next_actor_event();
			emit_full_state();
		}
	}

	/**
	 * @brief Acknowledges (closes) the currently shown monster-action popup.
	 *
	 * Any connected client may send this — the first one to arrive wins.
	 * Shows the next queued popup, if any, otherwise resumes the normal
	 * turn-advance flow (see advance_monster_popup()).
	 */
	void handle_ack_monster_popup(const nlohmann::json&)
	{
		if (!_engine || !_engine->has_pending_monster_popup()) { return; }
		advance_monster_popup();
	}

	// ── GM deck management commands ──────────────────────────────────────────

	void handle_deck_draw(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id = data.value("hero_id", std::string{});
		if (hero_id.empty()) { return; }
		_engine->draw_n_cards(hero_id, 1);
		emit_full_state();
	}

	void handle_deck_discard(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id = data.value("hero_id", std::string{});
		std::string card_id = data.value("card_id",  std::string{});
		if (hero_id.empty() || card_id.empty()) { return; }
		_engine->discard_card(hero_id, card_id);
		emit_full_state();
	}

	void handle_deck_take_discard(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id = data.value("hero_id", std::string{});
		if (hero_id.empty()) { return; }
		_engine->take_from_discard(hero_id);
		emit_full_state();
	}

	void handle_deck_reshuffle(const nlohmann::json& data)
	{
		if (!_engine) { return; }
		std::string hero_id = data.value("hero_id", std::string{});
		if (hero_id.empty()) { return; }
		_engine->reshuffle_discard(hero_id);
		emit_full_state();
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

		// Resolve display name and individual timeline position.
		std::string name_str = id;
		int actor_timeline   = 0;
		try
		{
			const gmActor::ActorStore& st = _engine->actor_store();
			if (kind == gmActor::ActorKind::HERO)
			{
				name_str       = st.hero(id).common.display_name;
				actor_timeline = st.hero(id).common.timeline_position;
			}
			else if (kind == gmActor::ActorKind::MONSTER_GROUP)
			{
				name_str       = st.monster_group(id).display_name;
				actor_timeline = st.monster_group(id).timeline_position;
			}
		}
		catch (...) {}

		nlohmann::json d;
		d["actor_id"]       = id;
		d["kind"]           = kind_str;
		d["actor_name"]     = name_str;
		d["mission_time"]   = _engine->mission_time();
		d["actor_timeline"] = actor_timeline;
		d["awaiting_confirmation"] = _engine->has_pending_turn_confirmation();
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

				// Discard pile: include actual card IDs so the GUI can show details.
				nlohmann::json discard_arr = nlohmann::json::array();
				for (const eldhom::CardId& cid : _engine->discard_cards(c.actor_id))
				{
					discard_arr.push_back(cid);
				}
				hj["discard_ids"] = discard_arr;

				// Cards played this turn (in played zone, not yet discarded).
				nlohmann::json played_arr = nlohmann::json::array();
				for (const eldhom::CardId& cid : _engine->played_cards(c.actor_id))
				{
					played_arr.push_back(cid);
				}
				hj["played_ids"] = played_arr;
			}
			catch (...)
			{
				hj["hand"]          = nlohmann::json::array();
				hj["deck_count"]    = 0;
				hj["discard_count"] = 0;
				hj["discard_ids"]   = nlohmann::json::array();
				hj["played_ids"]    = nlohmann::json::array();
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
			gj["monster_type"] = g.monster_type_id;

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

		// Special objects / items
		state["tesoro_carrier"] = _engine->tesoro_carrier();

		// Special objects — locked adjacency exposed for map display
		nlohmann::json spec_objs = nlohmann::json::array();
		for (const eldhom::SpecialObject& obj : _last_def.special_objects)
		{
			nlohmann::json sj;
			sj["object_id"]   = obj.object_id;
			sj["name"]        = obj.name;
			sj["type"]        = obj.type;
			sj["location_id"] = obj.location_id;
			nlohmann::json locked_adj = nlohmann::json::array();
			for (const std::pair<eldhom::LocationId, eldhom::LocationId>& p
				 : obj.on_interact.adjacency)
			{
				nlohmann::json pair_arr = nlohmann::json::array();
				pair_arr.push_back(p.first);
				pair_arr.push_back(p.second);
				locked_adj.push_back(pair_arr);
			}
			sj["locked_adjacency"] = locked_adj;
			spec_objs.push_back(sj);
		}
		state["special_objects"] = spec_objs;

		// Zone-boundary doors already opened by PGs (CLOSED_DOOR -> free passage)
		nlohmann::json opened_doors = nlohmann::json::array();
		for (const std::pair<eldhom::LocationId, eldhom::LocationId>& door
			 : _engine->opened_zone_doors())
		{
			nlohmann::json pair_arr = nlohmann::json::array();
			pair_arr.push_back(door.first);
			pair_arr.push_back(door.second);
			opened_doors.push_back(pair_arr);
		}
		state["opened_zone_doors"] = opened_doors;

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

	// Wall-clock timestamp of the currently shown monster-action popup, used
	// to enforce the 3-second auto-dismiss timeout from advance_auto()'s
	// existing 200ms polling loop (no extra thread/timer needed).
	std::chrono::steady_clock::time_point _monster_popup_shown_at;
};

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
	std::signal(SIGINT,  handle_signal);
	std::signal(SIGTERM, handle_signal);

	// data_dir: first positional arg or default relative to build location
	std::string data_dir = "GAME/Eldhom/data";
	if (argc >= 2 && argv[1][0] != '-') { data_dir = argv[1]; }

	// Optional CLI overrides (see file header): let eng_serve run several
	// independent engine instances at once, one per session, each pair on its
	// own dynamically-allocated ports. No arguments (desktop GUI launch
	// scripts) keeps the compiled-in defaults, unchanged.
	const uint16_t events_port   = read_port_option(argc, argv, "--events-port", eldhom::ports::EVENTS);
	const uint16_t commands_port = read_port_option(argc, argv, "--commands-port", eldhom::ports::COMMANDS);

	std::cout << "[EldhomEngine] Le Pergamene di Eldhom — CoreEngine P7\n";
	std::cout << "[EldhomEngine] Data dir : " << data_dir << "\n";
	std::cout << "[EldhomEngine] Events   : connecting to GUI on port "
	          << events_port << "\n";
	std::cout << "[EldhomEngine] Commands : listening on port "
	          << commands_port << "\n";

	EldhomServer server(data_dir, events_port);

	eldhom::EldhomCmdServer cmd_server(
		commands_port,
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

	std::cout << "[EldhomEngine] Ready. Waiting for GUI connection on port " << commands_port << "...\n";

	while (g_running)
	{
		server.advance_auto();
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	cmd_server.stop();
	std::cout << "[EldhomEngine] Stopped.\n";
	return 0;
}

