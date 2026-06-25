#ifndef GMRULES_CORE_RULEGROUP_HPP
#define GMRULES_CORE_RULEGROUP_HPP

/**
 * @file core/RuleGroup.hpp
 * @brief RuleGroup data struct and RuleGroupLifecycle enum for gmRules.
 *
 * A `RuleGroup` is a named set of rules that can be activated and
 * deactivated as a unit.  The `RuleGroupLifecycle` enum drives how the
 * `RuleGroupRegistry` manages activation / deactivation automatically
 * in response to token zone changes.
 *
 * This file has no dependencies beyond the C++17 standard library and
 * `core/Ids.hpp`.  It is intentionally free of `#include` directives
 * to other game-lib libraries.
 */

#include "Ids.hpp"

#include <string>
#include <vector>

namespace gmRules {

// ─────────────────────────────────────────────────────────────────────────────
// RuleGroupLifecycle
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum RuleGroupLifecycle
 * @brief Controls when a rule group's rules are active.
 *
 * | Value           | Active while …                                             |
 * |-----------------|-------------------------------------------------------------|
 * | `TRANSIENT`     | The token is in an *active* zone (PLAY_AREA or MEMORY).     |
 *                   | Rules are automatically deactivated when the token leaves.  |
 * | `PERSISTENT`    | From activation until explicit `deactivate()` call.         |
 *                   | Zone changes do not affect the activation state.            |
 * | `TRIGGER_BOUND` | Until the event named in `RuleGroup::trigger_event` fires.  |
 *                   | Rules are deactivated after the first matching event.        |
 */
enum class RuleGroupLifecycle
{
	TRANSIENT,      ///< Active only while the linked token is in PLAY_AREA / MEMORY.
	PERSISTENT,     ///< Active until explicitly deactivated regardless of zone.
	TRIGGER_BOUND   ///< Active until a specific named event fires once.
};

// ─────────────────────────────────────────────────────────────────────────────
// RuleGroup
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct RuleGroup
 * @brief Associates a set of rule IDs with a lifecycle policy.
 *
 * `group_id` is the primary key; it must be unique within a
 * `RuleGroupRegistry`.  The `rule_ids` list is the ordered sequence of
 * rules to activate / deactivate when the group is activated / deactivated.
 *
 * ## Example
 * @code
 *   gmRules::RuleGroup rg;
 *   rg.group_id  = "rg_village";
 *   rg.rule_ids  = {"r_add_buy", "r_add_coin"};
 *   rg.lifecycle = gmRules::RuleGroupLifecycle::TRANSIENT;
 * @endcode
 */
struct RuleGroup
{
	std::string            group_id;       ///< Unique identifier for this rule group.
	std::vector<RuleId>    rule_ids;        ///< Ordered list of rule IDs to manage.
	RuleGroupLifecycle     lifecycle       ///< Activation/deactivation policy.
		= RuleGroupLifecycle::TRANSIENT;
	std::string            trigger_event;  ///< Only meaningful when lifecycle == TRIGGER_BOUND.
	                                       ///< Names the event that auto-deactivates the group.
};

} // namespace gmRules

#endif // GMRULES_CORE_RULEGROUP_HPP
