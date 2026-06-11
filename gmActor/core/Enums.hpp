#ifndef GMACTOR_CORE_ENUMS_HPP
#define GMACTOR_CORE_ENUMS_HPP

/**
 * @file core/Enums.hpp
 * @brief All enumeration types used throughout gmActor.
 *
 * Centralized in one file to avoid repeated includes and to make the domain
 * vocabulary immediately visible.  Sub-module headers may re-export specific
 * enums via a thin include (e.g. `modifiers/ModifierOperation.hpp`).
 */

namespace gmActor {

// ─────────────────────────────────────────────────────────────────────────────
// Actor classification
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ActorKind
 * @brief Classifies the type of actor in the game state.
 *
 * Used by `ActorStore` to dispatch to the correct internal container and
 * to determine what operations are valid on an actor.
 */
enum class ActorKind {
    HERO,             ///< Player-controlled or player-facing hero
    ALLY_NPC,         ///< Allied non-player character
    MONSTER_INSTANCE, ///< Individual physical monster body (targetable)
    MONSTER_GROUP,    ///< Group that acts as a unit on the timeline
    BOSS,             ///< Boss extension (wraps a group + instance)
    MISSION_SYSTEM    ///< Scripted environment / event source
};

// ─────────────────────────────────────────────────────────────────────────────
// Spatial / tactical position
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum AreaPosition
 * @brief Internal tactical position within an area.
 *
 * Games that use front/back rank mechanics assign this value.
 * Games that do not use positional mechanics leave it as `NONE`.
 */
enum class AreaPosition {
    FRONTLINE, ///< Front rank — usually closer to enemies
    BACKLINE,  ///< Back rank — usually further from enemies
    NONE       ///< Position is not applicable or not set
};

// ─────────────────────────────────────────────────────────────────────────────
// Actor lifecycle
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ActorLifeState
 * @brief Coarse lifecycle state of a targetable actor.
 *
 * The game-specific engine decides how transitions between these states
 * occur.  `gmActor` stores and reports the value only.
 */
enum class ActorLifeState {
    ACTIVE,  ///< Alive and able to participate
    KO,      ///< Knocked out — alive but unable to act
    DEAD,    ///< Dead — permanently removed from combat
    REMOVED  ///< Removed from the scenario (fled, captured, etc.)
};

// ─────────────────────────────────────────────────────────────────────────────
// Item classification
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ItemKind
 * @brief Classifies the nature of an item definition.
 */
enum class ItemKind {
    WEAPON,       ///< Offensive hand-held weapon
    ARMOR,        ///< Protective body armour
    TRINKET,      ///< Accessory / passive item
    CONSUMABLE,   ///< One-use or limited-use item
    RELIC,        ///< Rare permanent item with special rules
    MISSION_ITEM, ///< Scenario-specific key item
    MATERIAL,     ///< Crafting material or resource token
    GENERIC       ///< Unclassified item
};

// ─────────────────────────────────────────────────────────────────────────────
// Equipment slots
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum EquipmentSlot
 * @brief Identifies a physical equipment slot on an actor.
 *
 * Slot legality (e.g. whether an actor may use OFF_HAND) is the
 * responsibility of the game-specific engine.
 */
enum class EquipmentSlot {
    MAIN_HAND, ///< Primary weapon / tool
    OFF_HAND,  ///< Secondary weapon / shield / off-hand tool
    ARMOR,     ///< Body armour
    TRINKET_1, ///< First trinket / accessory slot
    TRINKET_2, ///< Second trinket / accessory slot
    RELIC,     ///< Relic slot
    NONE       ///< No slot (unequipped reference)
};

// ─────────────────────────────────────────────────────────────────────────────
// Modifier operations
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ModifierOperation
 * @brief Mathematical operation applied by a modifier to a stat value.
 *
 * Evaluation order (see `apply_modifiers()`):
 * 1. SET   — last SET wins (by vector position).
 * 2. ADD / SUBTRACT
 * 3. MULTIPLY
 */
enum class ModifierOperation {
    ADD,      ///< Base value += modifier value
    SUBTRACT, ///< Base value -= modifier value
    MULTIPLY, ///< Base value *= modifier value
    SET       ///< Base value  = modifier value (last SET wins)
};

// ─────────────────────────────────────────────────────────────────────────────
// Modifier / status duration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ModifierDurationKind
 * @brief Describes when a modifier or status instance expires.
 *
 * The game-specific engine is responsible for checking expiry conditions
 * and calling the appropriate removal methods.  `gmActor` stores the value.
 */
enum class ModifierDurationKind {
    PERMANENT,                    ///< Never expires automatically
    UNTIL_NEXT_ACTIVATION,        ///< Expires at the bearer's next activation
    UNTIL_TARGET_NEXT_ACTIVATION, ///< Expires at the target's next activation
    UNTIL_TIME,                   ///< Expires at a specific time value
    WHILE_IN_AREA,                ///< Expires when bearer leaves the area
    WHILE_IN_POSITION,            ///< Expires when bearer changes position
    MANUAL_REMOVE                 ///< Only removed by explicit game rule
};

} // namespace gmActor

#endif // GMACTOR_CORE_ENUMS_HPP
