#ifndef GMRULES_CORE_RULEDEFINITION_HPP
#define GMRULES_CORE_RULEDEFINITION_HPP

/**
 * @file core/RuleDefinition.hpp
 * @brief Data struct describing one named rule: its preconditions and effects.
 *
 * A `RuleDefinition` is the bridge between a symbolic `RuleId` (e.g.
 * `"r_add_action_1"`) and the `EffectSpec` objects that `EffectResolver` can
 * actually execute.  Instances are stored in a `RuleBook` and looked up by
 * `RuleId` at resolution time.
 *
 * ## Relationship with other types
 * - `RuleGroup` (core/RuleGroup.hpp) — groups several `RuleId`s under a
 *   single activation key and lifecycle policy.
 * - `RuleBook` (core/RuleBook.hpp) — owns the `RuleDefinition` objects and
 *   drives resolution via `EffectResolver`.
 * - `EffectSpec` (effect/EffectSpec.hpp) — the executable leaf node.
 *
 * ## Lifecycle
 * `RuleDefinition` objects are immutable once registered in a `RuleBook`.
 * The `RuleBook` owns them by value; callers receive `const` references only.
 */

#include "gmRules/core/Ids.hpp"
#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/effect/EffectSpec.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @struct RuleDefinition
 * @brief Full definition of one named rule: preconditions + ordered effects.
 *
 * ## Example (corresponds to JSON `r_add_action_1`)
 * @code
 *   RuleDefinition def;
 *   def.rule_id     = "r_add_action_1";
 *   def.description = "+1 Azione";
 *   EffectSpec e;
 *   e.type   = EffectType::MODIFY_RESOURCE;
 *   e.target = TargetSpec{ TargetKind::ACTOR, TargetSelector::SELF };
 *   e.value  = "actions";
 *   e.amount = 1;
 *   def.effects.push_back(e);
 * @endcode
 */
struct RuleDefinition
{
	RuleId      rule_id;      ///< Unique identifier (matches keys in rule_groups.json).
	std::string description;  ///< Human-readable label for debugging and UI.

	/// Preconditions checked before any effect is applied.
	/// All must pass (implicit ALL_OF) for the rule to fire.
	/// May be empty — an empty list means "always fire".
	std::vector<ConditionSpec> preconditions;

	/// Ordered list of effects applied when the rule fires.
	/// Applied left-to-right; a non-optional failure stops the chain when
	/// `EffectSpec::stop_on_failure` is `true`.
	std::vector<EffectSpec>    effects;

	/** @brief Returns `true` if this definition carries at least one effect. */
	bool has_effects() const { return !effects.empty(); }

	/** @brief Returns `true` if this definition has at least one precondition. */
	bool has_preconditions() const { return !preconditions.empty(); }
};

} // namespace gmRules

#endif // GMRULES_CORE_RULEDEFINITION_HPP
