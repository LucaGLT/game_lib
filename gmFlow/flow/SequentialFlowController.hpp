#ifndef GMFLOW_SEQUENTIALFLOWCONTROLLER_HPP
#define GMFLOW_SEQUENTIALFLOWCONTROLLER_HPP

/**
 * @file flow/SequentialFlowController.hpp
 * @brief Concrete flow controller for classic sequential-turn games.
 *
 * SequentialFlowController drives a session through an ordered list of phases
 * with round and turn management following the @ref TurnPolicy and
 * @ref RoundPolicy supplied via @ref SessionConfig.
 *
 * Supported archetypes (out of the box, no subclassing required):
 * - **HeroQuest / Dungeon Crawler**: heroes act in fixed order, then monsters.
 * - **Risiko! / Wargame**: sequential player turns, no rounds (or capped rounds).
 * - **Game of Thrones board game**: phase-driven with sequential actor turns.
 *
 * For simultaneous turns (Gloomhaven) or complex reaction stacks, set the
 * appropriate `TurnPolicy` flags or provide a custom `IFlowController`.
 *
 * ### Construction
 * @code
 *   auto ctrl = std::make_unique<gmFlow::SequentialFlowController>(
 *       std::vector<std::unique_ptr<gmFlow::IPhase>>{
 *           std::make_unique<SetupPhase>(),
 *           std::make_unique<CombatPhase>(),
 *           std::make_unique<EndPhase>()
 *       });
 *
 *   gmFlow::GameSession session(config, std::move(ctrl));
 * @endcode
 */

#include "gmFlow/flow/IFlowController.hpp"
#include "gmFlow/flow/IPhase.hpp"
#include "gmFlow/flow/TurnPolicy.hpp"
#include "gmFlow/flow/RoundPolicy.hpp"
#include "gmFlow/flow/Turn.hpp"
#include "gmFlow/flow/Round.hpp"
#include "gmFlow/actions/ActionWindow.hpp"

#include <memory>
#include <vector>

namespace gmFlow {

/**
 * @class SequentialFlowController
 * @brief Default implementation of IFlowController for sequential-turn games.
 *
 * Phase order is determined by the vector passed to the constructor (index 0
 * is the first phase).  Turns are allocated to actors in the order they appear
 * in the @ref ActorRegistry (populated from @ref SessionConfig::actors).
 *
 * Override `determine_turn_order()` in a subclass to customise actor ordering
 * per phase (e.g. initiative-based in HeroQuest combat).
 */
class SequentialFlowController : public IFlowController {
public:
    /**
     * @brief Constructs the controller with phases, turn policy and round policy.
     *
     * @param phases      Phases to execute in order; must not be empty.
     * @param turn_policy Turn management flags (default: sequential, single action).
     * @param round_policy Round management flags (default: enabled, unlimited).
     * @throws std::invalid_argument if `phases` is empty.
     */
    explicit SequentialFlowController(
        std::vector<std::unique_ptr<IPhase>> phases,
        TurnPolicy                           turn_policy  = {},
        RoundPolicy                          round_policy = {});

    // IFlowController interface.
    void             start(GameContext& ctx) override;
    void             process(GameContext& ctx) override;
    bool             can_actor_act(const GameContext& ctx,
                                   const ActorId& actor) const override;
    void             on_action_completed(GameContext& ctx,
                                         const ActionResult& result) override;
    ValidationResult accept_action(GameContext& ctx,
                                   const ActorId& actor,
                                   std::unique_ptr<IAction> action) override;
    bool             is_session_complete(const GameContext& ctx) const override;

protected:
    /**
     * @brief Returns the ordered list of actor IDs for the current turn.
     *
     * Default implementation returns actors in registry insertion order.
     * Override to implement initiative, priority, or phase-specific ordering.
     *
     * @param ctx Read-only session context.
     * @return Ordered list of actor IDs for the new turn.
     */
    virtual std::vector<ActorId> determine_turn_order(const GameContext& ctx) const;

private:
    /// @brief Transitions the controller to the next phase, or ends the session.
    void advance_phase(GameContext& ctx);

    /// @brief Opens a new turn for the next actor in the turn order.
    void open_next_turn(GameContext& ctx);

    std::vector<std::unique_ptr<IPhase>> phases_;
    TurnPolicy                           turn_policy_;
    RoundPolicy                          round_policy_;
    std::size_t                          current_phase_index_ = 0;
    std::size_t                          current_actor_index_ = 0;
    int                                  round_index_         = 0;
    bool                                 session_complete_    = false;
    bool                                 rounds_exhausted_    = false;

    std::unique_ptr<ActionWindow>        current_window_;
};

} // namespace gmFlow

#endif // GMFLOW_SEQUENTIALFLOWCONTROLLER_HPP
