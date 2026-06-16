#ifndef GMACTOR_MODIFIERS_MODIFIER_HPP
#define GMACTOR_MODIFIERS_MODIFIER_HPP

/**
 * @file modifiers/Modifier.hpp
 * @brief Modifier definition, runtime instance, and evaluation helper.
 *
 * ## Modifier evaluation order
 *
 * `apply_modifiers()` evaluates modifiers against a base value in this order:
 * 1. **SET** — last SET (by vector position) overrides the base value.
 * 2. **ADD / SUBTRACT** — applied to the post-SET value.
 * 3. **MULTIPLY** — applied last, to the post-ADD/SUBTRACT value.
 *
 * Only modifiers whose `stat_key` matches the requested key are applied.
 * Modifiers for other stat keys are silently ignored.
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Enums.hpp"

#include <string>
#include <vector>

namespace gmActor {

// ─────────────────────────────────────────────────────────────────────────────
// ModifierDefinition
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Immutable template that describes a reusable modifier.
 *
 * Definitions are shared data (e.g. loaded from a data file).  At runtime a
 * `ModifierInstance` is created for each application to a specific actor.
 */
struct ModifierDefinition {
    ModifierId  id;                                    ///< Unique definition identifier
    std::string name;                                  ///< Human-readable label
    std::string stat_key;                              ///< Target stat (e.g. "base_damage")
    ModifierOperation operation = ModifierOperation::ADD; ///< Mathematical operation
    double value = 0.0;                                ///< Modifier magnitude
    std::vector<Tag> tags;                             ///< Classification tags
};

// ─────────────────────────────────────────────────────────────────────────────
// ModifierInstance
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Mutable runtime application of a modifier to an actor.
 *
 * An instance is created each time a modifier is applied.  Multiple instances
 * of the same `ModifierId` may exist on one actor (e.g. from different sources).
 */
struct ModifierInstance {
    ModifierId  id;                                    ///< Matches a ModifierDefinition id
    SourceId    source_id;                             ///< Who applied this modifier
    std::string stat_key;                              ///< Target stat (copied from definition)
    ModifierOperation operation = ModifierOperation::ADD; ///< Operation (copied from definition)
    double value = 0.0;                                ///< Magnitude (may differ from definition)
    ModifierDurationKind duration_kind = ModifierDurationKind::MANUAL_REMOVE; ///< Expiry rule
    int expires_at_time = -1;                          ///< Game-time tick for UNTIL_TIME, else -1
};

// ─────────────────────────────────────────────────────────────────────────────
// Evaluator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Applies all relevant modifiers to a base value and returns the result.
 *
 * Only modifiers whose `stat_key == requested_stat_key` are included.
 * Evaluation order: SET (last wins) → ADD/SUBTRACT → MULTIPLY.
 *
 * @param base_value          Starting value before any modifiers.
 * @param stat_key            The stat being evaluated (e.g. `"base_damage"`).
 * @param modifiers           List of active modifier instances on the actor.
 * @return                    Final computed value.
 */
double apply_modifiers(double base_value,
                       const std::string& stat_key,
                       const std::vector<ModifierInstance>& modifiers);

} // namespace gmActor

#endif // GMACTOR_MODIFIERS_MODIFIER_HPP
