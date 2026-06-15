#ifndef GMRULES_STATUS_STATUSDEFINITION_HPP
#define GMRULES_STATUS_STATUSDEFINITION_HPP

/**
 * @file status/StatusDefinition.hpp
 * @brief Immutable definition of a reusable status effect.
 *
 * `StatusDefinition` objects are typically loaded at game startup from
 * data files and remain constant during play.
 * `StatusInstance` objects are the mutable runtime applications of a definition.
 */

#include "gmRules/core/Ids.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/status/Modifier.hpp"
#include "gmRules/status/StackingPolicy.hpp"
#include "gmRules/status/Duration.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Immutable description of a status effect.
 *
 * Effects in the lifecycle hooks (`on_apply`, `on_remove`, etc.) are resolved
 * by `StatusEngine` via `EffectResolver`.
 */
struct StatusDefinition
{
    StatusId    id;           ///< Unique status identifier
    std::string name;         ///< Human-readable label
    std::vector<std::string> tags; ///< Classification tags

    // ── Lifecycle effects ─────────────────────────────────────────────────────
    std::vector<EffectSpec> on_apply;            ///< Effects resolved when status is applied
    std::vector<EffectSpec> on_remove;           ///< Effects resolved when status is removed
    std::vector<EffectSpec> on_activation_start; ///< Effects at start of owner's activation
    std::vector<EffectSpec> on_activation_end;   ///< Effects at end of owner's activation
    std::vector<EffectSpec> continuous_effects;  ///< Reserved for future use in V1

    // ── Modifiers ─────────────────────────────────────────────────────────────
    std::vector<Modifier> modifiers; ///< Persistent stat modifiers while status is active

    // ── Stacking and duration ─────────────────────────────────────────────────
    StackingPolicy stacking_policy;   ///< How re-application is handled
    DurationSpec   default_duration;  ///< Default duration given to new instances
};

} // namespace gmRules

#endif // GMRULES_STATUS_STATUSDEFINITION_HPP
