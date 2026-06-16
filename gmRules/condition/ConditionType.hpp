#ifndef GMRULES_CONDITION_CONDITIONTYPE_HPP
#define GMRULES_CONDITION_CONDITIONTYPE_HPP

/**
 * @file condition/ConditionType.hpp
 * @brief Enumeration of atomic condition types and composite operators.
 */

namespace gmRules {

/**
 * @brief Atomic condition types for use in `ConditionSpec`.
 */
enum class ConditionType
{
    ALWAYS,              ///< Unconditionally true
    NEVER,               ///< Unconditionally false

    // Actor-based
    ACTOR_EXISTS,        ///< Actor with `subject_id` exists in context
    ACTOR_HAS_STATUS,    ///< Actor has status with `value` (status ID)
    ACTOR_HAS_TAG,       ///< Actor has tag `value`
    ACTOR_HP_AT_OR_BELOW,///< Actor current HP ≤ `amount`
    ACTOR_HP_AT_OR_ABOVE,///< Actor current HP ≥ `amount`
    ACTOR_IN_LOCATION,   ///< Actor is in location `value`
    ACTOR_IN_POSITION,   ///< Actor is in area-position `value`

    // Target-based (requires resolved targets in context)
    TARGET_EXISTS,       ///< At least one target is resolved
    TARGET_HAS_STATUS,   ///< First target has status `value`
    TARGET_HAS_TAG,      ///< First target has tag `value`

    // Location-based
    LOCATION_EXISTS,     ///< Location `subject_id` exists
    LOCATION_HAS_TAG,    ///< Location `subject_id` has tag `value`
    LOCATION_IS_ADJACENT,///< Locations `subject_id` and `target_id` are adjacent

    // Deck / card (Chapter 5 — gmAlea)
    DECK_HAS_AT_LEAST,   ///< Deck `subject_id` zone `value` has ≥ `amount` cards
    CARD_IN_ZONE,        ///< Card `target_id` is in deck `subject_id` zone `value`

    // Resources
    RESOURCE_AT_LEAST,   ///< Actor resource `value` on `subject_id` ≥ `amount`

    // Map advanced (Chapter 6 — gmMap)
    LOCATION_IS_REACHABLE, ///< Location `target_id` is reachable from `subject_id`
    LOCATION_HAS_LOS,      ///< Line-of-sight from `subject_id` to `target_id`
    MOVE_COST_AT_MOST,     ///< Move cost from `subject_id` to `target_id` ≤ `amount`

    CUSTOM               ///< Game-specific; always fails unless context handles it
};

/**
 * @brief Logical operator for composite conditions.
 */
enum class CompositeOperator
{
    ALL_OF, ///< All children must be true (AND)
    ANY_OF, ///< At least one child must be true (OR)
    NONE_OF,///< No child must be true (NOR)
    NOT     ///< Single child must be false (NOT — uses `children[0]` only)
};

} // namespace gmRules

#endif // GMRULES_CONDITION_CONDITIONTYPE_HPP
