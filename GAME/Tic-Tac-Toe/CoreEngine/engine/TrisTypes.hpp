#ifndef GMTRIS_TRISTYPES_HPP
#define GMTRIS_TRISTYPES_HPP

/**
 * @file engine/TrisTypes.hpp
 * @brief Shared value types, enums and protocol identifiers for the Tris engine.
 */

#include <cstdint>
#include <string>

namespace gmTris
{

/// @brief Content of a single board cell.
enum class Mark
{
	EMPTY, ///< Cell not yet played.
	X,     ///< Cell played by Player X.
	O      ///< Cell played by Player O.
};

/// @brief High-level phase of the match.
enum class Phase
{
	BOOTSTRAP,    ///< Engine initialising, no session yet.
	PLAYER1_TURN, ///< Waiting for the player that owns turn 1.
	PLAYER2_TURN, ///< Waiting for the player that owns turn 2.
	GAME_OVER     ///< Match finished (win or draw).
};

/// @brief How the first player is chosen at the start of a match.
enum class StarterMode
{
	FIXED_X, ///< Player X always starts.
	DICE_1D2 ///< A 1d2 roll decides who starts.
};

/// @brief Network ports used by the Engine↔GUI bridge.
namespace ports
{
/// @brief Port where the GUI listens for engine events (Engine is client).
/// @note 9000 is commonly occupied by local proxy/agent software on Windows,
///       so the events channel uses 9100 to avoid that conflict.
constexpr uint16_t EVENTS = 9100;
/// @brief Port where the engine listens for GUI commands (Engine is server).
constexpr uint16_t COMMANDS = 9001;
} // namespace ports

/// @brief typeId strings for commands received from the GUI.
namespace command_id
{
constexpr const char* MOVE     = "gmTris.move";
constexpr const char* NEW_GAME = "gmTris.new_game";
} // namespace command_id

/// @brief typeId strings for events emitted towards the GUI.
namespace event_id
{
constexpr const char* ACTOR_SNAPSHOT  = "gmActor.snapshot";
constexpr const char* MAP_SNAPSHOT    = "gmMap.snapshot";
constexpr const char* SESSION_STARTED = "gmFlow.session.started";
constexpr const char* PHASE_CHANGED   = "gmFlow.session.phase_changed";
constexpr const char* STATUS_ADDED    = "gmActor.actor.status_added";
constexpr const char* STATUS_REMOVED  = "gmActor.actor.status_removed";
constexpr const char* CELL_CHANGED    = "gmMap.cell_changed";
constexpr const char* GAME_WON        = "gmRules.game_won";
constexpr const char* GAME_DRAW       = "gmRules.game_draw";
constexpr const char* INVALID_MOVE    = "gmTris.invalid_move";
constexpr const char* DICE_ROLLED     = "gmAlea.dice_rolled";
} // namespace event_id

/// @brief Returns the single-character symbol for a mark ("X", "O" or "").
inline std::string mark_to_string(Mark mark)
{
	switch (mark)
	{
	case Mark::X:
		return "X";
	case Mark::O:
		return "O";
	default:
		return "";
	}
}

/// @brief Returns a stable string identifier for a phase.
inline std::string phase_to_string(Phase phase)
{
	switch (phase)
	{
	case Phase::PLAYER1_TURN:
		return "PLAYER1_TURN";
	case Phase::PLAYER2_TURN:
		return "PLAYER2_TURN";
	case Phase::GAME_OVER:
		return "GAME_OVER";
	default:
		return "BOOTSTRAP";
	}
}

} // namespace gmTris

#endif // GMTRIS_TRISTYPES_HPP
