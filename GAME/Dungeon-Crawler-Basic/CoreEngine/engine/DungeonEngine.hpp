#ifndef GMDUNGEONBASIC_DUNGEONENGINE_HPP
#define GMDUNGEONBASIC_DUNGEONENGINE_HPP

/**
 * @file engine/DungeonEngine.hpp
 * @brief Facade/Mediator that coordinates all Dungeon Crawler subsystems.
 *
 * DungeonEngine owns one wrapper per reused library (world/gmMap, actors/gmActor,
 * flow/gmFlow, rules/gmRules, log/gmLog) and the outbound GUI bridge
 * (gmDispatch). It exposes three entry points: @ref start_game to load a dungeon
 * and begin a session, @ref handle_command to process GUI commands, and
 * @ref advance_turn to drive automatic phase transitions.
 *
 * @note Not thread-safe. All calls must come from the same thread (the CmdServer
 *       callback thread in normal operation).
 */

#include "actions/ActionV1.hpp"
#include "actors/ActorRoster.hpp"
#include "bridge/GuiBridge.hpp"
#include "engine/DungeonTypes.hpp"
#include "flow/TurnFlow.hpp"
#include "log/GameLog.hpp"
#include "rules/DungeonRuleAdapter.hpp"
#include "world/DungeonMap.hpp"
#include "world/DungeonMapLoader.hpp"

#include "gmInteraction/InteractableObjectStore.hpp"
#include "gmSave/json.hpp"

#include <string>

namespace gmDungeonBasic
{

/**
 * @brief Central facade/mediator for a Dungeon Crawler Basic session.
 *
 * Owns all domain subsystems and drives the main game loop. Emits JSON events
 * via GuiBridge for every state change that the GUI must reflect.
 */
class DungeonEngine
{
public:
	/**
	 * @brief Constructs the engine and readies all subsystems.
	 *
	 * No network connection is established at construction time; GuiBridge
	 * connects lazily on the first emitted event.
	 */
	DungeonEngine();

	/**
	 * @brief Loads a dungeon map from JSON and starts a new session.
	 *
	 * Resets all subsystems, loads the map via DungeonMapLoader, populates the
	 * ActorRoster, resets the TurnFlow and emits the initial snapshots and
	 * session-started event.
	 *
	 * @param map_file  Path to the JSON map file (see info/wire-contract-v1.md).
	 * @throws std::runtime_error  If the map file cannot be loaded.
	 */
	void start_game(const std::string& map_file = "");

	/**
	 * @brief Processes one command received from the GUI.
	 *
	 * Dispatches to the appropriate private handler based on @p typeId.
	 * Unknown typeIds are silently ignored.
	 *
	 * @param typeId  Command type identifier (see @ref gmDungeonBasic::command_id).
	 * @param data    Command payload object.
	 */
	void handle_command(const std::string& typeId, const nlohmann::json& data);

	/**
	 * @brief Advances the game phase automatically where no hero input is needed.
	 *
	 * Called by the main loop to trigger monster-turn logic and phase
	 * transitions that do not depend on GUI commands.
	 */
	void advance_turn();

private:
	// ── Command handlers ──────────────────────────────────────────────────────

	/// @brief Handles a dungeon.move command.
	void handle_move(const nlohmann::json& data);

	/// @brief Handles a dungeon.heal command.
	void handle_heal(const nlohmann::json& data);

	/// @brief Handles a dungeon.equip command.
	void handle_equip(const nlohmann::json& data);

	/// @brief Handles a dungeon.end_turn command.
	void handle_end_turn(const nlohmann::json& data);

	/// @brief Handles a gmMap.area.info.request command (view-only, no gameplay effect).
	void handle_area_info_request(const nlohmann::json& data);

	// ── Snapshot builders ─────────────────────────────────────

	/// @brief Builds the full map snapshot payload for the GUI.
	nlohmann::json build_map_snapshot() const;

	/// @brief Builds the full actor roster snapshot payload for the GUI.
	nlohmann::json build_actor_snapshot() const;

	/// @brief Builds the area-info payload (actors + interactables) for an area.
	nlohmann::json build_area_info(const std::string& area_id) const;

	/**
	 * @brief Rebuilds the interactable object store from the loaded map.
	 *
	 * Creates one @c gmInteraction interactable per room tag and registers its
	 * id on the map location, so area-info is sourced natively from gmMap +
	 * gmInteraction rather than from ad-hoc tag scanning.
	 */
	void rebuild_interactables();

	// ── Subsystems ────────────────────────────────────────────────────────────

	DungeonMap          _map;      ///< gmMap-backed dungeon map.
	DungeonMapLoader    _loader;   ///< JSON map file parser.
	ActorRoster         _actors;   ///< gmActor-backed actor roster.
	TurnFlow            _flow;     ///< gmFlow-backed turn/round manager.
	DungeonRuleAdapter  _rules;    ///< gmRules-backed action validator.
	ActionV1            _actions;  ///< Action executor for v1 actions.
	GameLog             _log;      ///< gmLog-backed session log.
	GuiBridge           _gui;      ///< Outbound event channel to the GUI.

	gmInteraction::InteractableObjectStore _interactables; ///< Interactable object data.
	gmMap::InteractableObjectId _next_interactable_id = 1; ///< Id allocator for interactables.

	GamePhase   _phase   = GamePhase::BOOTSTRAP;   ///< Current game phase.
	GameOutcome _outcome = GameOutcome::NONE;       ///< Outcome once GAME_OVER.
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_DUNGEONENGINE_HPP
