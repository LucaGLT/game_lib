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
    REVIVE_ACTOR,     ///< Revive a previously downed actor
    CHANGE_TEAM,      ///< Move an actor to another team/faction

    // ── Actor resources / equipment ───────────────────────────────────────────
    MODIFY_RESOURCE,  ///< Apply signed delta to an actor resource
    SET_RESOURCE_MAX, ///< Set max value for an actor resource
    EQUIP_ITEM,       ///< Equip item identified in `value`
    UNEQUIP_ITEM,     ///< Unequip item slot identified in `value`

    // ── Extended deck/dice actions ────────────────────────────────────────────
    SHUFFLE_ZONE,         ///< Shuffle a zone in a deck
    LOOK_TOP_CARD,        ///< Peek top N cards without drawing
    LOOK_BOTTOM_CARD,     ///< Peek bottom N cards without drawing
    SELECT_SPECIFIC_CARD, ///< Select one specific card by ID
    DISCARD_RANDOM,       ///< Discard random cards from a zone
    PLACE_ON_TOP,         ///< Place card on top of deck
    PLACE_ON_BOTTOM,      ///< Place card on bottom of deck
    ROLL_DICE,            ///< Roll dice through runtime random provider

    // ── Extended map actions ──────────────────────────────────────────────────
    SET_LOCATION_PASSABLE,///< Toggle location passability
    ADD_LOCATION_TAG,     ///< Add a tag to a location
    REMOVE_LOCATION_TAG,  ///< Remove a tag from a location
    SET_LOCATION_OWNER,   ///< Assign location owner/controller
    CREATE_BARRIER,       ///< Create topological barrier
    REMOVE_BARRIER,       ///< Remove topological barrier
    SPAWN_INTERACTABLE,   ///< Spawn map interactable object
    DESPAWN_INTERACTABLE, ///< Despawn map interactable object

    // ── Events / escape hatch ─────────────────────────────────────────────────
    EMIT_EVENT,       ///< Emit a `RuleEvent` without mutating state
    MANUAL_EFFECT,    ///< Escape hatch: emit event, do not mutate state (D6)
    CUSTOM            ///< Game-specific effect delegated through `RuleContext`
};

/**
 * @brief Returns a stable, human-readable name for an effect type.
 */
inline const char* effect_type_name(EffectType type)
{
    if (type == EffectType::DEAL_DAMAGE) return "DEAL_DAMAGE";
    if (type == EffectType::HEAL) return "HEAL";
    if (type == EffectType::MOVE_ACTOR) return "MOVE_ACTOR";
    if (type == EffectType::SHIFT_POSITION) return "SHIFT_POSITION";
    if (type == EffectType::DRAW_CARDS) return "DRAW_CARDS";
    if (type == EffectType::DISCARD_CARDS) return "DISCARD_CARDS";
    if (type == EffectType::MOVE_CARD_TO_ZONE) return "MOVE_CARD_TO_ZONE";
    if (type == EffectType::APPLY_STATUS) return "APPLY_STATUS";
    if (type == EffectType::REMOVE_STATUS) return "REMOVE_STATUS";
    if (type == EffectType::ADD_MODIFIER) return "ADD_MODIFIER";
    if (type == EffectType::REMOVE_MODIFIER) return "REMOVE_MODIFIER";
    if (type == EffectType::ADD_TAG) return "ADD_TAG";
    if (type == EffectType::REMOVE_TAG) return "REMOVE_TAG";
    if (type == EffectType::SET_STATE) return "SET_STATE";
    if (type == EffectType::SPAWN_ACTOR) return "SPAWN_ACTOR";
    if (type == EffectType::DESPAWN_ACTOR) return "DESPAWN_ACTOR";
    if (type == EffectType::REVIVE_ACTOR) return "REVIVE_ACTOR";
    if (type == EffectType::CHANGE_TEAM) return "CHANGE_TEAM";
    if (type == EffectType::MODIFY_RESOURCE) return "MODIFY_RESOURCE";
    if (type == EffectType::SET_RESOURCE_MAX) return "SET_RESOURCE_MAX";
    if (type == EffectType::EQUIP_ITEM) return "EQUIP_ITEM";
    if (type == EffectType::UNEQUIP_ITEM) return "UNEQUIP_ITEM";
    if (type == EffectType::SHUFFLE_ZONE) return "SHUFFLE_ZONE";
    if (type == EffectType::LOOK_TOP_CARD) return "LOOK_TOP_CARD";
    if (type == EffectType::LOOK_BOTTOM_CARD) return "LOOK_BOTTOM_CARD";
    if (type == EffectType::SELECT_SPECIFIC_CARD) return "SELECT_SPECIFIC_CARD";
    if (type == EffectType::DISCARD_RANDOM) return "DISCARD_RANDOM";
    if (type == EffectType::PLACE_ON_TOP) return "PLACE_ON_TOP";
    if (type == EffectType::PLACE_ON_BOTTOM) return "PLACE_ON_BOTTOM";
    if (type == EffectType::ROLL_DICE) return "ROLL_DICE";
    if (type == EffectType::SET_LOCATION_PASSABLE) return "SET_LOCATION_PASSABLE";
    if (type == EffectType::ADD_LOCATION_TAG) return "ADD_LOCATION_TAG";
    if (type == EffectType::REMOVE_LOCATION_TAG) return "REMOVE_LOCATION_TAG";
    if (type == EffectType::SET_LOCATION_OWNER) return "SET_LOCATION_OWNER";
    if (type == EffectType::CREATE_BARRIER) return "CREATE_BARRIER";
    if (type == EffectType::REMOVE_BARRIER) return "REMOVE_BARRIER";
    if (type == EffectType::SPAWN_INTERACTABLE) return "SPAWN_INTERACTABLE";
    if (type == EffectType::DESPAWN_INTERACTABLE) return "DESPAWN_INTERACTABLE";
    if (type == EffectType::EMIT_EVENT) return "EMIT_EVENT";
    if (type == EffectType::MANUAL_EFFECT) return "MANUAL_EFFECT";
    if (type == EffectType::CUSTOM) return "CUSTOM";
    return "UNKNOWN_EFFECT";
}

} // namespace gmRules

#endif // GMRULES_EFFECT_EFFECTTYPE_HPP
