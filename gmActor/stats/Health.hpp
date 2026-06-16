#ifndef GMACTOR_STATS_HEALTH_HPP
#define GMACTOR_STATS_HEALTH_HPP

/**
 * @file stats/Health.hpp
 * @brief Free-function helpers for safe HP mutation on ActorStateCommon.
 *
 * ## Rules
 * - HP is always clamped to `[0, max_hp]`.
 * - Negative `amount` values are treated as zero (no-op).
 * - These helpers **do not** transition `life_state`.  The game engine
 *   decides whether 0 HP means KO, death, or something else.
 * - An actor with `max_hp == 0` is considered to have no health
 *   (`has_health()` returns false).
 */

#include "gmActor/actors/ActorStateCommon.hpp"

namespace gmActor {

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Returns true if the actor has meaningful HP (`max_hp > 0`).
 */
bool has_health(const ActorStateCommon& actor);

/**
 * @brief Returns `max_hp - current_hp` (always ≥ 0).
 */
int missing_hp(const ActorStateCommon& actor);

/**
 * @brief Returns true if the actor is alive (`life_state == ACTIVE`).
 */
bool is_alive(const ActorStateCommon& actor);

/**
 * @brief Returns true if the actor is knocked out (`life_state == KO`).
 */
bool is_ko(const ActorStateCommon& actor);

// ─────────────────────────────────────────────────────────────────────────────
// Mutation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Sets HP to `value`, clamped to `[0, max_hp]`.
 *
 * @param actor Actor to modify.
 * @param value New HP value (clamped).
 */
void set_hp(ActorStateCommon& actor, int value);

/**
 * @brief Reduces HP by `amount`, clamped to a minimum of 0.
 *
 * Negative amounts are treated as zero (no-op).
 *
 * @param actor  Actor to modify.
 * @param amount Damage amount (≥ 0).
 */
void damage_hp(ActorStateCommon& actor, int amount);

/**
 * @brief Increases HP by `amount`, clamped to a maximum of `max_hp`.
 *
 * Negative amounts are treated as zero (no-op).
 *
 * @param actor  Actor to modify.
 * @param amount Healing amount (≥ 0).
 */
void heal_hp(ActorStateCommon& actor, int amount);

} // namespace gmActor

#endif // GMACTOR_STATS_HEALTH_HPP
