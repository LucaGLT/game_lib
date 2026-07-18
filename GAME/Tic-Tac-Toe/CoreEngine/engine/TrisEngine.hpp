#ifndef GMTRIS_TRISENGINE_HPP
#define GMTRIS_TRISENGINE_HPP

/**
 * @file engine/TrisEngine.hpp
 * @brief Facade/Mediator that coordinates all Tris subsystems.
 *
 * TrisEngine owns one wrapper per reused library (board/gmMap, players/gmActor,
 * flow/gmFlow, rules/gmRules, alea/gmAlea, logging/gmLog) and the outbound GUI
 * bridge (gmDispatch). It exposes two entry points: @ref start_game to begin a
 * match and @ref handle_command to process GUI commands, emitting events for
 * every state change.
 */

#include "alea/Starter.hpp"
#include "board/Board.hpp"
#include "bridge/GuiBridge.hpp"
#include "engine/TrisTypes.hpp"
#include "flow/TurnFlow.hpp"
#include "logging/GameLog.hpp"
#include "players/Players.hpp"
#include "rules/WinRules.hpp"

#include "gmSave/json.hpp"

#include <string>

namespace gmTris
{

/**
 * @class TrisEngine
 * @brief Central coordinator for a Tic-Tac-Toe match.
 */
class TrisEngine
{
  public:
	/**
	 * @brief Constructs the engine and connects the outbound GUI bridge.
	 *
	 * @param events_port TCP port of the GUI/eng_serve event listener (default
	 *        @ref gmTris::ports::EVENTS). Overridable so multiple engine
	 *        instances can each target a different listener — this is what
	 *        lets eng_serve run one engine per user session, each pair on its
	 *        own dynamically-allocated ports (see GAME/Tic-Tac-Toe/WebApp).
	 */
	explicit TrisEngine(uint16_t events_port = ports::EVENTS);

	/**
	 * @brief Starts a new match.
	 *
	 * Resets the board, chooses the first player and emits the initial
	 * snapshots and session events.
	 *
	 * @param mode How the first player is chosen.
	 */
	void start_game(StarterMode mode = StarterMode::FIXED_X);

	/**
	 * @brief Processes one command received from the GUI.
	 *
	 * @param typeId Command type id (see @ref gmTris::command_id).
	 * @param data   Command payload.
	 */
	void handle_command(const std::string& typeId, const nlohmann::json& data);

  private:
	/// @brief Handles a @c gmTris.move command.
	void handle_move(const nlohmann::json& data);

	/// @brief Maps the active mark to its turn phase relative to the starter.
	Phase phase_for(Mark mark) const;

	/// @brief Parses a "X"/"O" string into a Mark (EMPTY when unrecognised).
	static Mark parse_mark(const std::string& symbol);

	// ── Event payload builders ────────────────────────────────────────────────
	nlohmann::json build_actor_snapshot() const;
	nlohmann::json build_board_snapshot() const;

	// ── Subsystems (one per reused library) ───────────────────────────────────
	Board     _board;
	Players   _players;
	TurnFlow  _flow;
	WinRules  _rules;
	Starter   _starter;
	GameLog   _log;
	GuiBridge _gui;

	Mark _starter_mark = Mark::X; ///< Mark of player 1 (the starter).
};

} // namespace gmTris

#endif // GMTRIS_TRISENGINE_HPP
