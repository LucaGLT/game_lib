#ifndef GMRULES_TARGET_TARGETRESOLVER_HPP
#define GMRULES_TARGET_TARGETRESOLVER_HPP

/**
 * @file target/TargetResolver.hpp
 * @brief Resolves a `TargetSpec` against game state.
 *
 * `TargetResolver` is **side-effect-free**.  It only reads from `RuleContext`.
 *
 * ## Resolution flow
 * 1. Apply `TargetSelector` to build a candidate list.
 * 2. Apply `RangeType` to filter by location distance.
 * 3. Apply `required_tags` / `forbidden_tags` filters.
 * 4. Apply `allow_self` filter.
 * 5. Return `TargetResult`.
 */

#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/target/TargetResult.hpp"
#include "gmRules/core/RuleContext.hpp"

#include <vector>

namespace gmRules {

/**
 * @brief Resolves `TargetSpec` instances against a `RuleContext`.
 *
 * Stateless — all methods are `const`.
 */
class TargetResolver
{
public:
    /**
     * @brief Resolves a target spec.
     *
     * @param spec              Describes how to select targets.
     * @param source_actor_id   Actor originating the effect.
     * @param selected_targets  Pre-selected targets from the caller
     *                          (used for SELECTED_ACTOR / MANUAL).
     * @param ctx               Read-only access to game state.
     * @return                  Resolved target list or failure.
     */
    TargetResult resolve(const TargetSpec& spec,
                         const ActorId& source_actor_id,
                         const std::vector<TargetRef>& selected_targets,
                         const RuleContext& ctx) const;
};

} // namespace gmRules

#endif // GMRULES_TARGET_TARGETRESOLVER_HPP
