#ifndef GMRULES_EFFECT_EFFECTTYPE_HPP
#define GMRULES_EFFECT_EFFECTTYPE_HPP

/**
 * @file effect/EffectType.hpp
 * @brief Enumeration of effect types supported by `EffectResolver`.
 */

namespace gmRules {

/**
 * @brief Classifies the mutation an effect performs.
 */
enum class EffectType
{
    // ── Health / position ─────────────────────────────────────────────────────
    DEAL_DAMAGE,      ///< Reduce target HP by `amount`
    HEAL,             ///< Increase target HP by `amount`
    MOVE_ACTOR,       ///< Move target actor to location `value`
    SHIFT_POSITION,   ///< Change actor's area position (front/back)

    // ── Card management ───────────────────────────────────────────────────────
    DRAW_CARDS,       ///< Draw `amount` cards from deck `value`
    DISCARD_CARDS,    ///< Discard `amount` cards from hand (game-specific)
    MOVE_CARD_TO_ZONE,///< Move card `value` to zone named in `value`

    // ── Status / modifiers ────────────────────────────────────────────────────
    APPLY_STATUS,     ///< Apply status `value` to target
    REMOVE_STATUS,    ///< Remove status `value` from target
    ADD_MODIFIER,     ///< Add a modifier (game-specific delegation)
    REMOVE_MODIFIER,  ///< Remove a modifier by ID

    // ── Tags / state ──────────────────────────────────────────────────────────
    ADD_TAG,          ///< Add tag `value` to target actor
    REMOVE_TAG,       ///< Remove tag `value` from target actor
    SET_STATE,        ///< Set an opaque state value (game-specific)

    // ── Actor lifecycle ───────────────────────────────────────────────────────
    SPAWN_ACTOR,      ///< Spawn actor described in `value` (game-specific)
    DESPAWN_ACTOR,    ///< Remove actor from play

    // ── Events / escape hatch ─────────────────────────────────────────────────
    EMIT_EVENT,       ///< Emit a `RuleEvent` without mutating state
    MANUAL_EFFECT,    ///< Escape hatch: emit event, do not mutate state (D6)
    CUSTOM            ///< Game-specific effect delegated through `RuleContext`
};

} // namespace gmRules

#endif // GMRULES_EFFECT_EFFECTTYPE_HPP
