#ifndef GMRULES_CONDITION_TRIGGERSPEC_HPP
#define GMRULES_CONDITION_TRIGGERSPEC_HPP

/**
 * @file condition/TriggerSpec.hpp
 * @brief Event-based activation specification for reactions and status effects.
 *
 * `TriggerSpec` describes what kind of game event must occur for a
 * rule to activate.  It is intentionally event-system-agnostic.
 * A game-specific adapter translates `gmFlow`, `gmDispatch`, or custom
 * events into trigger checks.
 */

#include "gmRules/condition/ConditionSpec.hpp"

#include <vector>

namespace gmRules {

/**
 * @brief Classifies what kind of event activates a trigger.
 */
enum class TriggerType
{
    ON_ACTION_SUBMITTED,  ///< An action was submitted to the flow controller
    ON_ACTION_COMPLETED,  ///< An action finished executing
    ON_ACTION_SKIPPED,    ///< An action window closed with no executed action
    ON_ACTION_WINDOW_OPENED, ///< Action-selection window opened
    ON_ACTION_WINDOW_CLOSED, ///< Action-selection window closed
    ON_TURN_STARTED,      ///< A turn started for an actor
    ON_TURN_COMPLETED,    ///< A turn completed for an actor
    ON_ROUND_STARTED,     ///< A round started
    ON_ROUND_COMPLETED,   ///< A round completed
    ON_PHASE_CHANGED,     ///< The game phase changed
    ON_GAME_STATE_CHANGED,///< Global game state changed
    ON_CARD_PLAYED,       ///< A card was played from hand
    ON_TOKEN_PRE_DRAW,    ///< Before a token/card draw is resolved
    ON_TOKEN_DRAWN,       ///< After token/card draw resolved
    ON_DICE_PRE_ROLL,     ///< Before dice roll is resolved
    ON_DICE_ROLLED,       ///< After dice roll is resolved
    ON_ALEA_RESOLVED,     ///< After random subsystem has resolved
    ON_ACTOR_DAMAGED,     ///< An actor received damage
    ON_ACTOR_MOVED,       ///< An actor moved to a new location
    ON_ACTOR_APPROACHED,  ///< An actor approached a location threshold
    ON_ACTOR_LEFT_LOCATION, ///< An actor left a location
    ON_ACTOR_SPAWNED,     ///< An actor spawned in world
    ON_ACTOR_DESPAWNED,   ///< An actor despawned from world
    ON_ACTOR_HP_CHANGED,  ///< Actor HP changed value
    ON_ACTOR_DIED,        ///< Actor entered dead state
    ON_ACTOR_REVIVED,     ///< Actor revived from dead state
    ON_RESOURCE_CHANGED,  ///< Actor resource changed
    ON_ITEM_EQUIPPED,     ///< Item equipped on actor
    ON_ITEM_UNEQUIPPED,   ///< Item unequipped from actor
    ON_STATUS_APPLIED,    ///< A status was applied to an actor
    ON_TIME_REACHED,      ///< A specific time value was reached
    ON_LOCATION_ENTERED,  ///< An actor entered a location
    ON_LOCATION_STATE_CHANGED, ///< Location state/tag changed
    ON_PATH_BLOCKED,      ///< Traversal path became blocked
    ON_LOS_CHANGED,       ///< Line-of-sight relation changed
    CUSTOM                ///< Game-specific trigger
};

/**
 * @brief Returns a stable, human-readable name for a trigger type.
 */
inline const char* trigger_type_name(TriggerType type)
{
    if (type == TriggerType::ON_ACTION_SUBMITTED) return "ON_ACTION_SUBMITTED";
    if (type == TriggerType::ON_ACTION_COMPLETED) return "ON_ACTION_COMPLETED";
    if (type == TriggerType::ON_ACTION_SKIPPED) return "ON_ACTION_SKIPPED";
    if (type == TriggerType::ON_ACTION_WINDOW_OPENED) return "ON_ACTION_WINDOW_OPENED";
    if (type == TriggerType::ON_ACTION_WINDOW_CLOSED) return "ON_ACTION_WINDOW_CLOSED";
    if (type == TriggerType::ON_TURN_STARTED) return "ON_TURN_STARTED";
    if (type == TriggerType::ON_TURN_COMPLETED) return "ON_TURN_COMPLETED";
    if (type == TriggerType::ON_ROUND_STARTED) return "ON_ROUND_STARTED";
    if (type == TriggerType::ON_ROUND_COMPLETED) return "ON_ROUND_COMPLETED";
    if (type == TriggerType::ON_PHASE_CHANGED) return "ON_PHASE_CHANGED";
    if (type == TriggerType::ON_GAME_STATE_CHANGED) return "ON_GAME_STATE_CHANGED";
    if (type == TriggerType::ON_CARD_PLAYED) return "ON_CARD_PLAYED";
    if (type == TriggerType::ON_TOKEN_PRE_DRAW) return "ON_TOKEN_PRE_DRAW";
    if (type == TriggerType::ON_TOKEN_DRAWN) return "ON_TOKEN_DRAWN";
    if (type == TriggerType::ON_DICE_PRE_ROLL) return "ON_DICE_PRE_ROLL";
    if (type == TriggerType::ON_DICE_ROLLED) return "ON_DICE_ROLLED";
    if (type == TriggerType::ON_ALEA_RESOLVED) return "ON_ALEA_RESOLVED";
    if (type == TriggerType::ON_ACTOR_DAMAGED) return "ON_ACTOR_DAMAGED";
    if (type == TriggerType::ON_ACTOR_MOVED) return "ON_ACTOR_MOVED";
    if (type == TriggerType::ON_ACTOR_APPROACHED) return "ON_ACTOR_APPROACHED";
    if (type == TriggerType::ON_ACTOR_LEFT_LOCATION) return "ON_ACTOR_LEFT_LOCATION";
    if (type == TriggerType::ON_ACTOR_SPAWNED) return "ON_ACTOR_SPAWNED";
    if (type == TriggerType::ON_ACTOR_DESPAWNED) return "ON_ACTOR_DESPAWNED";
    if (type == TriggerType::ON_ACTOR_HP_CHANGED) return "ON_ACTOR_HP_CHANGED";
    if (type == TriggerType::ON_ACTOR_DIED) return "ON_ACTOR_DIED";
    if (type == TriggerType::ON_ACTOR_REVIVED) return "ON_ACTOR_REVIVED";
    if (type == TriggerType::ON_RESOURCE_CHANGED) return "ON_RESOURCE_CHANGED";
    if (type == TriggerType::ON_ITEM_EQUIPPED) return "ON_ITEM_EQUIPPED";
    if (type == TriggerType::ON_ITEM_UNEQUIPPED) return "ON_ITEM_UNEQUIPPED";
    if (type == TriggerType::ON_STATUS_APPLIED) return "ON_STATUS_APPLIED";
    if (type == TriggerType::ON_TIME_REACHED) return "ON_TIME_REACHED";
    if (type == TriggerType::ON_LOCATION_ENTERED) return "ON_LOCATION_ENTERED";
    if (type == TriggerType::ON_LOCATION_STATE_CHANGED) return "ON_LOCATION_STATE_CHANGED";
    if (type == TriggerType::ON_PATH_BLOCKED) return "ON_PATH_BLOCKED";
    if (type == TriggerType::ON_LOS_CHANGED) return "ON_LOS_CHANGED";
    if (type == TriggerType::CUSTOM) return "CUSTOM";
    return "UNKNOWN_TRIGGER";
}

/**
 * @brief Describes when a rule should fire automatically.
 */
struct TriggerSpec
{
    TriggerType                type       = TriggerType::CUSTOM; ///< Event type
    std::vector<ConditionSpec> conditions; ///< Additional conditions that must hold
};

} // namespace gmRules

#endif // GMRULES_CONDITION_TRIGGERSPEC_HPP
