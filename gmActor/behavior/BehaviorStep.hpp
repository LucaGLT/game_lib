#ifndef GMACTOR_BEHAVIOR_BEHAVIORSTEP_HPP
#define GMACTOR_BEHAVIOR_BEHAVIORSTEP_HPP

/**
 * @file behavior/BehaviorStep.hpp
 * @brief Atomic step within a behavior card sequence.
 *
 * `BehaviorStep` is a plain data object that describes one action a monster
 * group will attempt during its turn (or reaction).  It carries no behavior
 * of its own — all interpretation is delegated to the `StepExecutor` callback
 * provided by the game engine.
 *
 * ### Field semantics
 *
 * | Field            | Meaning                                                        |
 * |------------------|----------------------------------------------------------------|
 * | `effect_type`    | Opaque string key passed to the engine executor (e.g. `"DEAL_DAMAGE"`, `"MOVE"`). |
 * | `amount`         | Numeric parameter for the effect (damage, range, distance).   |
 * | `value`          | String parameter for the effect (direction, status ID, …).   |
 * | `timeline_cost`  | Timeline ticks the **group** pays after this step completes.  |
 * |                  | Paid once per step regardless of how many members execute it. |
 * | `optional`       | When `true`, if the step cannot be executed it is silently    |
 * |                  | skipped.  When `false`, a failed step triggers fallback logic. |
 */

#include <string>

namespace gmActor {

/**
 * @struct BehaviorStep
 * @brief One atomic action in a behavior card's step list.
 */
struct BehaviorStep
{
	std::string effect_type;   ///< Engine-level effect key (e.g. "DEAL_DAMAGE").
	int         amount       = 0;     ///< Numeric parameter (damage, distance, …)
	std::string value;                ///< String parameter (status ID, direction, …)
	int         timeline_cost = 1;   ///< Timeline ticks paid by the group after this step.
	bool        optional     = false; ///< True if the step may be skipped without fallback.
};

} // namespace gmActor

#endif // GMACTOR_BEHAVIOR_BEHAVIORSTEP_HPP
