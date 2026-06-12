#ifndef GMFLOW_IPHASE_HPP
#define GMFLOW_IPHASE_HPP

/**
 * @file flow/IPhase.hpp
 * @brief Interface for a discrete phase within a game session.
 *
 * A "phase" is any named segment of a game session with distinct rules about
 * which actions are available (Setup, Planning, Combat, Resolution, End-of-Round,
 * etc.).  Game-specific code implements IPhase for each game phase and registers
 * the phases with the @ref IFlowController.
 *
 * ### Minimal implementation
 * @code
 *   class SetupPhase : public gmFlow::IPhase {
 *   public:
 *       gmFlow::PhaseId id() const override { return "SETUP"; }
 *
 *       void on_enter(gmFlow::GameContext& ctx) override {
 *           // distribute starting cards, place tokens, etc.
 *       }
 *       void on_exit(gmFlow::GameContext& ctx) override {
 *           // validate setup rules, clean up temporary state
 *       }
 *
 *       std::vector<std::unique_ptr<gmFlow::IAction>>
 *       available_actions(const gmFlow::GameContext& ctx,
 *                         const gmFlow::ActorId& actor) const override
 *       {
 *           // return actions the actor may perform during setup
 *           return {};
 *       }
 *
 *       bool is_complete(const gmFlow::GameContext& ctx) const override {
 *           return static_cast<const MyState&>(ctx.state()).setup_done;
 *       }
 *   };
 * @endcode
 */

#include "gmFlow/core/Ids.hpp"
#include "gmFlow/actions/IAction.hpp"

#include <memory>
#include <vector>

// Forward declaration — avoids pulling in the full GameContext chain here.
namespace gmFlow { class GameContext; }

namespace gmFlow {

/**
 * @class IPhase
 * @brief Pure-virtual interface for a game phase.
 *
 * The flow engine calls `on_enter()` when transitioning into a phase and
 * `on_exit()` when leaving it.  Between those calls, it queries
 * `available_actions()` and `is_complete()` each tick.
 */
class IPhase {
public:
    virtual ~IPhase() = default;

    /// @brief Returns the unique identifier for this phase.
    virtual PhaseId id() const = 0;

    /**
     * @brief Called once when the flow engine enters this phase.
     *
     * Implementations should perform phase-start setup: emit events,
     * initialise phase-local state, open the first ActionWindow, etc.
     *
     * @param ctx Mutable session context.
     */
    virtual void on_enter(GameContext& ctx) = 0;

    /**
     * @brief Called once just before the flow engine leaves this phase.
     *
     * Implementations should perform cleanup: finalise scoring,
     * close any open ActionWindows, persist phase results, etc.
     *
     * @param ctx Mutable session context.
     */
    virtual void on_exit(GameContext& ctx) = 0;

    /**
     * @brief Returns the actions currently available to the given actor.
     *
     * Called each tick by the @ref IFlowController to populate the UI action
     * palette and to validate incoming submissions.  Must be const.
     *
     * @param ctx   Read-only session context.
     * @param actor The actor querying for available actions.
     * @return Zero or more action prototypes the actor may submit.
     */
    virtual std::vector<std::unique_ptr<IAction>>
        available_actions(const GameContext& ctx,
                          const ActorId& actor) const = 0;

    /**
     * @brief Returns true when this phase's exit condition is satisfied.
     *
     * Called each tick; when it returns true the flow engine calls `on_exit()`
     * and advances to the next phase.
     *
     * @param ctx Read-only session context.
     * @return true if the phase is complete.
     */
    virtual bool is_complete(const GameContext& ctx) const = 0;
};

} // namespace gmFlow

#endif // GMFLOW_IPHASE_HPP
