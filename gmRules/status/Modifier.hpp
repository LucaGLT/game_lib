#ifndef GMRULES_STATUS_MODIFIER_HPP
#define GMRULES_STATUS_MODIFIER_HPP

/**
 * @file status/Modifier.hpp
 * @brief Persistent stat modifier carried by a status or item.
 *
 * `gmRules` defines modifier data.  Actual stat recalculation is
 * delegated to `gmActor` or game-specific code that calls `gmActor::apply_modifiers()`.
 */

#include "gmRules/condition/ConditionSpec.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Mathematical operation for a modifier.
 */
enum class ModifierOp
{
    ADD,      ///< result += amount
    SUBTRACT, ///< result -= amount
    MULTIPLY, ///< result *= (amount / 100.0) — or raw if game interprets directly
    SET,      ///< result = amount (last SET wins)
    MIN,      ///< result = max(result, amount) — floor
    MAX       ///< result = min(result, amount) — cap
};

/**
 * @brief A persistent modifier attached to a status or other rule object.
 *
 * `conditions` is evaluated each time the modifier is applied;
 * if conditions fail, the modifier is skipped for that evaluation.
 */
struct Modifier
{
    std::string stat_id;             ///< Target stat key (e.g. "base_damage", "hp_max")
    ModifierOp  op     = ModifierOp::ADD; ///< Operation
    int         amount = 0;          ///< Modifier value

    std::vector<ConditionSpec> conditions; ///< Optional guard conditions
};

} // namespace gmRules

#endif // GMRULES_STATUS_MODIFIER_HPP
