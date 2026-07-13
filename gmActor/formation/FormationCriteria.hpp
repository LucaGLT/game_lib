#ifndef GMACTOR_FORMATION_FORMATIONCRITERIA_HPP
#define GMACTOR_FORMATION_FORMATIONCRITERIA_HPP

/**
 * @file formation/FormationCriteria.hpp
 * @brief Predefined `FormationCriterion` factories for `FormationResolver`.
 *
 * Each factory function returns a `FormationCriterion` that can be passed
 * directly to `FormationResolver`.  Criteria are composable: pass several in
 * a `std::vector` to build a prioritised comparator chain.
 *
 * ### Built-in criteria
 *
 * | Factory                      | Semantics                                       |
 * |------------------------------|-------------------------------------------------|
 * | `by_highest_hp()`            | Actor with more `current_hp` is preferred.      |
 * | `by_lowest_timeline()`       | Actor with the lower `timeline_position` first. |
 * | `random(seed)`               | Randomised last-resort tiebreaker.              |
 *
 * ### Usage
 *
 * @code
 *   using namespace gmActor::FormationCriteria;
 *
 *   gmActor::FormationResolver resolver({
 *       by_highest_hp(),
 *       by_lowest_timeline(),
 *       random(42)
 *   });
 * @endcode
 *
 * ### Adding game-specific criteria
 *
 * Game code implements its own `FormationCriterion` lambda and passes it to
 * the resolver vector — no changes to the library are needed.
 *
 * @code
 *   // "Actor with more cards in hand goes first" — game-specific criterion.
 *   auto by_most_cards = [&deck_state](const gmActor::ActorId& a,
 *                                      const gmActor::ActorId& b,
 *                                      const gmActor::ActorStore&) -> bool
 *   {
 *       return deck_state.hand_size(a) > deck_state.hand_size(b);
 *   };
 * @endcode
 */

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/core/Ids.hpp"

#include <functional>

namespace gmActor {

/**
 * @brief Comparator type used by `FormationResolver`.
 *
 * Returns `true` if actor `a` should be ranked **before** actor `b`
 * (i.e. `a` is a better candidate to be moved to the frontline first).
 * Returns `false` if `b` is strictly preferred, or if both actors are
 * indistinguishable by this criterion (the resolver falls through to the
 * next criterion).
 */
using FormationCriterion = std::function<bool(const ActorId& a,
                                              const ActorId& b,
                                              const ActorStore& store)>;

namespace FormationCriteria {

// ── Built-in factory functions ────────────────────────────────────────────────

/**
 * @brief Criterion: prefer the actor with the **highest** current HP.
 *
 * Both actors must expose `ActorStateCommon` (hero, ally, or monster
 * instance).  If `store.common(id)` throws (e.g. the actor is a
 * MonsterGroup), the criterion returns `false` for that pair.
 */
FormationCriterion by_highest_hp();

/**
 * @brief Criterion: prefer the actor with the **lowest** `timeline_position`.
 *
 * Useful when the formation resolver is used together with
 * `gmFlow::TimelineFlowController`.
 *
 * Both actors must expose `ActorStateCommon`.
 */
FormationCriterion by_lowest_timeline();

/**
 * @brief Criterion: randomised tiebreaker using a simple LCG seeded with `seed`.
 *
 * Intended as the last entry in a criteria chain.  Because the same seed
 * produces the same order for the same pair of actor IDs, the result is
 * deterministic and reproducible given the same seed.
 *
 * @param seed  Initial seed for the LCG (pass any non-zero value).
 */
FormationCriterion random(unsigned seed);

} // namespace FormationCriteria
} // namespace gmActor

#endif // GMACTOR_FORMATION_FORMATIONCRITERIA_HPP
