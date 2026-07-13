#ifndef GMRULES_EFFECT_EFFECTSPEC_HPP
#define GMRULES_EFFECT_EFFECTSPEC_HPP

/**
 * @file effect/EffectSpec.hpp
 * @brief Describes one rule effect and its application parameters.
 */

#include "gmRules/effect/EffectType.hpp"
#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/core/Ids.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Full specification of a rule effect.
 *
 * ## `value` field semantics by `EffectType`
 * | EffectType           | `value` meaning                              |
 * |----------------------|----------------------------------------------|
 * | MOVE_ACTOR           | Destination location ID                      |
 * | DRAW_CARDS           | Deck ID                                      |
 * | MOVE_CARD_TO_ZONE    | `"card_id:zone_name"` or two fields          |
 * | APPLY_STATUS         | Status definition ID                         |
 * | REMOVE_STATUS        | Status ID to remove                          |
 * | ADD_TAG / REMOVE_TAG | Tag string                                   |
 * | EMIT_EVENT / MANUAL_EFFECT | Event type string                      |
 * | MODIFY_RESOURCE      | Resource ID                                  |
 * | SET_RESOURCE_MAX     | Resource ID                                  |
 * | SET_ACTOR_RESOURCE   | Resource ID to set to `amount`               |
 * | TRIGGER_RULE         | Rule ID to chain via ctx.apply_extended_effect |
 * | SCALE_EFFECT         | Resource ID to read from source actor        |
 * | DELAY_EFFECT         | Inner effect type name; `amount` = its amount|
 * | others               | Not used — leave empty                       |
 *
 * ## `chain_count` field semantics
 * | EffectType   | `chain_count` meaning                            |
 * |--------------|--------------------------------------------------|
 * | CHAIN_EFFECT | Max number of additional bounce targets (≥0)     |
 * | DELAY_EFFECT | Turns to wait before the inner effect fires (≥0) |
 * | others       | Unused; defaults to 0                            |
 */
struct EffectSpec
{
    EffectType type      = EffectType::CUSTOM; ///< What the effect does
    std::string source_id;                     ///< Who owns/triggers this effect

    TargetSpec target;                         ///< Target specification

    int         amount = 0;     ///< Numeric operand (damage, heal amount, card count)
    std::string value;          ///< String operand (status ID, zone, tag, etc.)

    /// CHAIN_EFFECT: max bounce count; DELAY_EFFECT: turns delay; others: unused.
    int chain_count = 0;

    std::vector<ConditionSpec> conditions; ///< Preconditions checked before applying

    bool optional        = false; ///< If `true`, failure becomes a warning
    bool stop_on_failure = true;  ///< If `true`, `resolve_many()` stops on failure
};

} // namespace gmRules

#endif // GMRULES_EFFECT_EFFECTSPEC_HPP
