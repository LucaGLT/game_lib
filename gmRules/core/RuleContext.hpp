#ifndef GMRULES_CORE_RULECONTEXT_HPP
#define GMRULES_CORE_RULECONTEXT_HPP

/**
 * @file core/RuleContext.hpp
 * @brief Abstract adapter interface between gmRules and game state.
 *
 * Game-specific code implements `RuleContext` by delegating to `gmActor`,
 * `gmMap`, `gmDeck`/`gmCompDeck`, and its own state container.
 *
 * `gmRules` must not depend on any concrete game state or on `gmFlow`.
 *
 * ## Invariants
 * - `ConditionEvaluator` and `TargetResolver` only call `const` methods.
 * - `EffectResolver` and `StatusEngine` call mutating methods.
 * - All IDs are `std::string` aliases.
 */

#include "gmRules/core/Ids.hpp"
#include "gmRules/core/RuleResult.hpp"
#include "gmRules/core/RuleEvent.hpp"
#include "gmRules/status/StatusInstance.hpp"

#include <string>
#include <vector>

namespace gmRules {

struct EffectSpec;
struct TargetRef;

/**
 * @brief Abstract adapter that gives `gmRules` access to game state.
 *
 * Implement this interface in game-specific code by delegating to `gmActor`,
 * `gmMap`, `gmDeck`/`gmCompDeck`, and the relevant event bus.
 */
class RuleContext
{
public:
    virtual ~RuleContext() = default;

    // ── Actor queries (const) ─────────────────────────────────────────────────

    /** @brief Returns `true` if the actor is registered in the context. */
    virtual bool has_actor(const ActorId& actor_id) const = 0;

    /** @brief Returns `true` if the actor has the given tag. */
    virtual bool actor_has_tag(const ActorId& actor_id,
                               const std::string& tag) const = 0;

    /** @brief Returns the actor's current HP. */
    virtual int actor_current_hp(const ActorId& actor_id) const = 0;

    /** @brief Returns the actor's maximum HP. */
    virtual int actor_max_hp(const ActorId& actor_id) const = 0;

    /** @brief Returns `true` if the actor currently has the given status. */
    virtual bool actor_has_status(const ActorId& actor_id,
                                  const StatusId& status_id) const = 0;

    /**
     * @brief Returns all status instance IDs currently on the actor.
     * @param actor_id Target actor.
     */
    virtual std::vector<StatusInstanceId>
    statuses_on_actor(const ActorId& actor_id) const = 0;

    /** @brief Returns `true` if the two actors are allied. */
    virtual bool are_allies(const ActorId& a, const ActorId& b) const = 0;

    /** @brief Returns `true` if the two actors are enemies. */
    virtual bool are_enemies(const ActorId& a, const ActorId& b) const = 0;

    // ── Actor mutation ────────────────────────────────────────────────────────

    /**
     * @brief Applies a signed HP delta to the actor.
     *
     * The context implementation is responsible for clamping and
     * life-state transitions.
     * @param actor_id Target actor.
     * @param delta    Positive = heal, negative = damage.
     */
    virtual void modify_actor_hp(const ActorId& actor_id, int delta) = 0;

    /** @brief Adds a tag to the actor. */
    virtual void add_actor_tag(const ActorId& actor_id,
                               const std::string& tag) = 0;

    /** @brief Removes a tag from the actor. */
    virtual void remove_actor_tag(const ActorId& actor_id,
                                  const std::string& tag) = 0;

    // ── Status mutation ───────────────────────────────────────────────────────

    /**
     * @brief Adds a status instance to the context (via actor status container).
     * @param status Fully constructed `StatusInstance`.
     */
    virtual void add_status_instance(const StatusInstance& status) = 0;

    /**
     * @brief Removes a status instance by its unique instance ID.
     * @param instance_id Unique identifier of the instance to remove.
     */
    virtual void remove_status_instance(const StatusInstanceId& instance_id) = 0;

    // ── Location queries (const) ──────────────────────────────────────────────

    /** @brief Returns `true` if the location exists in the context. */
    virtual bool has_location(const LocationId& location_id) const = 0;

    /** @brief Returns the current location ID of the actor. */
    virtual LocationId actor_location(const ActorId& actor_id) const = 0;

    /** @brief Returns `true` if the two locations are directly adjacent. */
    virtual bool are_locations_adjacent(const LocationId& a,
                                        const LocationId& b) const = 0;

    /**
     * @brief Returns the shortest-path distance between two locations.
     *
     * Returns -1 if unreachable.
     */
    virtual int distance_between_locations(const LocationId& a,
                                           const LocationId& b) const = 0;

    /** @brief Returns `true` if the location has the given tag. */
    virtual bool location_has_tag(const LocationId& location_id,
                                  const std::string& tag) const = 0;

    /**
     * @brief Returns all actor IDs currently in the given location.
     * @param location_id Target location.
     */
    virtual std::vector<ActorId>
    actors_in_location(const LocationId& location_id) const = 0;

    // ── Location mutation ─────────────────────────────────────────────────────

    /**
     * @brief Moves the actor to a new location.
     * @param actor_id    Actor to move.
     * @param location_id Destination location.
     */
    virtual void move_actor_to_location(const ActorId& actor_id,
                                        const LocationId& location_id) = 0;

    // ── Deck / card access ────────────────────────────────────────────────────

    /** @brief Returns `true` if the deck exists. */
    virtual bool has_deck(const DeckId& deck_id) const = 0;

    /**
     * @brief Draws the requested number of cards from the deck.
     * @param deck_id Deck to draw from.
     * @param amount  Number of cards to draw.
     * @return        Vector of drawn card IDs (may be shorter than `amount`).
     */
    virtual std::vector<CardId> draw_cards(const DeckId& deck_id,
                                           int amount) = 0;

    /**
     * @brief Moves a card from its current zone to the named zone.
     * @param deck_id   Deck instance.
     * @param card_id   Card to move.
     * @param zone_name Destination zone name.
     */
    virtual RuleResult move_card_to_zone(const DeckId& deck_id,
                                         const CardId& card_id,
                                         const std::string& zone_name) = 0;

    // ── Events ────────────────────────────────────────────────────────────────

    /**
     * @brief Emits a rule event through the game's event bus.
     *
     * The context implementation may forward this to `gmDispatch`,
     * a custom observer list, or simply log it.
     * @param event Event to emit.
     */
    virtual void emit_event(const RuleEvent& event) = 0;

    /**
     * @brief Applies an extended effect not handled by gmRules V1 core.
     *
     * This hook is used for cross-library effects introduced by the DSL
     * evolution (gmFlow lifecycle, gmAlea advanced actions, gmMap advanced
     * topology changes, and game-specific effects).
     *
     * @param effect          Effect specification to apply.
     * @param target          Resolved effect target.
     * @param source_actor_id Source actor originating the effect.
     * @param out_event       Optional emitted event to append to result.
     * @return                `ok()` on success, `fail(...)` otherwise.
     */
    virtual RuleResult apply_extended_effect(const EffectSpec& effect,
                                             const TargetRef& target,
                                             const ActorId& source_actor_id,
                                             RuleEvent* out_event) = 0;
};

} // namespace gmRules

#endif // GMRULES_CORE_RULECONTEXT_HPP
