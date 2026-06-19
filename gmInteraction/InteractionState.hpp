#ifndef GMINTERACTION_INTERACTIONSTATE_HPP
#define GMINTERACTION_INTERACTIONSTATE_HPP

/**
 * @file InteractionState.hpp
 * @brief Lifecycle state of an interactable object.
 *
 * The state is intentionally small and game-independent. Game-specific
 * semantics (what "USED" means for a chest versus a lever) live in the game
 * layer, not in this library.
 */

#include <string>

#include "GmInteractionError.hpp"

namespace gmInteraction
{

/**
 * @brief Coarse lifecycle state shared by every interactable object.
 */
enum class InteractionState
{
	IDLE,      ///< Ready, not yet interacted with.
	ACTIVE,    ///< Currently engaged / in use.
	USED,      ///< Already consumed or triggered.
	LOCKED,    ///< Present but not interactable until unlocked.
	DISABLED   ///< Inactive; ignored by interaction queries.
};

/**
 * @brief Converts an @ref InteractionState to its canonical string form.
 *
 * @param state  State value to convert.
 * @return       Upper-case string identifier (e.g. @c "IDLE").
 */
inline std::string interaction_state_to_string(InteractionState state)
{
	switch (state)
	{
		case InteractionState::IDLE:     return "IDLE";
		case InteractionState::ACTIVE:   return "ACTIVE";
		case InteractionState::USED:     return "USED";
		case InteractionState::LOCKED:   return "LOCKED";
		case InteractionState::DISABLED: return "DISABLED";
	}
	return "IDLE";
}

/**
 * @brief Parses an @ref InteractionState from its canonical string form.
 *
 * @param text  String identifier produced by @ref interaction_state_to_string.
 * @return      The matching state value.
 * @throws EInteractionError  If @p text is not a recognised state.
 */
inline InteractionState interaction_state_from_string(const std::string& text)
{
	if (text == "IDLE")     return InteractionState::IDLE;
	if (text == "ACTIVE")   return InteractionState::ACTIVE;
	if (text == "USED")     return InteractionState::USED;
	if (text == "LOCKED")   return InteractionState::LOCKED;
	if (text == "DISABLED") return InteractionState::DISABLED;
	throw EInteractionError("unknown InteractionState string: '" + text + "'");
}

} // namespace gmInteraction

#endif // GMINTERACTION_INTERACTIONSTATE_HPP
