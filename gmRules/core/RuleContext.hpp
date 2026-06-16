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

    /**
     * @brief Returns the current value of a named actor resource.
     * @param actor_id    Target actor.
     * @param resource_id Identifier of the resource (e.g. "mana", "stamina").
     */
    virtual int actor_resource(const ActorId& actor_id,
                               const std::string& resource_id) const = 0;

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

    // ── Actor lifecycle (Chapter 4 — gmActor) ────────────────────────────────

    /**
     * @brief Spawns a new actor into the context.
     * @param actor_id  Identifier to assign to the spawned actor.
     * @param spec_json Serialised actor specification (game-specific).
     */
    virtual void spawn_actor(const ActorId& actor_id,
                             const std::string& spec_json) = 0;

    /** @brief Removes an actor from the context permanently. */
    virtual void despawn_actor(const ActorId& actor_id) = 0;

    /** @brief Revives a previously dead/downed actor. */
    virtual void revive_actor(const ActorId& actor_id) = 0;

    /**
     * @brief Moves an actor to a different team or faction.
     * @param actor_id Target actor.
     * @param team_id  New team identifier.
     */
    virtual void change_actor_team(const ActorId& actor_id,
                                   const std::string& team_id) = 0;

    // ── Actor resources (Chapter 4 — gmActor) ────────────────────────────────

    /**
     * @brief Applies a signed delta to a named actor resource.
     * @param actor_id    Target actor.
     * @param resource_id Resource identifier.
     * @param delta       Signed delta value.
     */
    virtual void modify_resource(const ActorId& actor_id,
                                 const std::string& resource_id,
                                 int delta) = 0;

    /**
     * @brief Sets the maximum value of a named actor resource.
     * @param actor_id    Target actor.
     * @param resource_id Resource identifier.
     * @param max_value   New maximum value.
     */
    virtual void set_resource_max(const ActorId& actor_id,
                                  const std::string& resource_id,
                                  int max_value) = 0;

    // ── Equipment (Chapter 4 — gmActor) ──────────────────────────────────────

    /**
     * @brief Equips an item on an actor.
     * @param actor_id Target actor.
     * @param item_id  Identifier of the item to equip.
     */
    virtual void equip_item(const ActorId& actor_id,
                            const std::string& item_id) = 0;

    /**
     * @brief Unequips an item from an actor slot.
     * @param actor_id Target actor.
     * @param slot_id  Equipment slot identifier.
     */
    virtual void unequip_item(const ActorId& actor_id,
                              const std::string& slot_id) = 0;

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

    // ── Advanced map queries (Chapter 6 — gmMap) ─────────────────────────────

    /**
     * @brief Returns `true` if a path exists from `from` to `to`.
     * @param from Origin location.
     * @param to   Destination location.
     */
    virtual bool is_location_reachable(const LocationId& from,
                                       const LocationId& to) const = 0;

    /**
     * @brief Returns `true` if `from` has an unobstructed line of sight to `to`.
     * @param from Observer location.
     * @param to   Target location.
     */
    virtual bool has_line_of_sight(const LocationId& from,
                                   const LocationId& to) const = 0;

    /**
     * @brief Returns the movement cost from `from` to `to`.
     *
     * Returns -1 if unreachable.
     * @param from Origin location.
     * @param to   Destination location.
     */
    virtual int move_cost_between(const LocationId& from,
                                  const LocationId& to) const = 0;

    // ── Location mutation ─────────────────────────────────────────────────────

    /**
     * @brief Moves the actor to a new location.
     * @param actor_id    Actor to move.
     * @param location_id Destination location.
     */
    virtual void move_actor_to_location(const ActorId& actor_id,
                                        const LocationId& location_id) = 0;

    // ── Advanced map mutations (Chapter 6 — gmMap) ───────────────────────────

    /**
     * @brief Sets the passability of a location.
     * @param location_id Target location.
     * @param passable    `true` = passable, `false` = blocked.
     */
    virtual void set_location_passable(const LocationId& location_id,
                                       bool passable) = 0;

    /**
     * @brief Adds a tag to a location.
     * @param location_id Target location.
     * @param tag         Tag string to add.
     */
    virtual void add_location_tag(const LocationId& location_id,
                                  const std::string& tag) = 0;

    /**
     * @brief Removes a tag from a location.
     * @param location_id Target location.
     * @param tag         Tag string to remove.
     */
    virtual void remove_location_tag(const LocationId& location_id,
                                     const std::string& tag) = 0;

    /**
     * @brief Assigns an owner/controller to a location.
     * @param location_id Target location.
     * @param owner_id    Owner actor or team identifier.
     */
    virtual void set_location_owner(const LocationId& location_id,
                                    const std::string& owner_id) = 0;

    /**
     * @brief Creates a directional topological barrier between two locations.
     * @param from       Origin location.
     * @param to         Destination location.
     * @param barrier_id Unique identifier for the barrier.
     */
    virtual void create_barrier(const LocationId& from,
                                const LocationId& to,
                                const std::string& barrier_id) = 0;

    /**
     * @brief Removes a previously created barrier.
     * @param barrier_id Barrier identifier returned from `create_barrier`.
     */
    virtual void remove_barrier(const std::string& barrier_id) = 0;

    /**
     * @brief Spawns an interactable object at a location.
     * @param location_id Target location.
     * @param spec_json   Serialised interactable specification.
     */
    virtual void spawn_interactable(const LocationId& location_id,
                                    const std::string& spec_json) = 0;

    /**
     * @brief Removes an interactable object from the map.
     * @param interactable_id Identifier of the interactable to remove.
     */
    virtual void despawn_interactable(const std::string& interactable_id) = 0;

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

    // ── Extended deck/dice operations (Chapter 5 — gmAlea) ───────────────────

    /**
     * @brief Returns the number of cards in a named zone of a deck.
     * @param deck_id   Deck to query.
     * @param zone_name Zone name; empty string queries the full deck.
     */
    virtual int deck_zone_count(const DeckId& deck_id,
                                const std::string& zone_name) const = 0;

    /**
     * @brief Returns `true` if the card is currently in the given zone.
     * @param deck_id   Deck to query.
     * @param card_id   Card to locate.
     * @param zone_name Zone name to check against.
     */
    virtual bool card_in_zone(const DeckId& deck_id,
                              const CardId& card_id,
                              const std::string& zone_name) const = 0;

    /**
     * @brief Shuffles the cards within a named deck zone.
     * @param deck_id   Target deck.
     * @param zone_name Zone to shuffle.
     */
    virtual void shuffle_zone(const DeckId& deck_id,
                              const std::string& zone_name) = 0;

    /**
     * @brief Returns the top N card IDs without removing them.
     * @param deck_id Target deck.
     * @param count   Number of cards to peek.
     */
    virtual std::vector<CardId> look_top_cards(const DeckId& deck_id,
                                               int count) const = 0;

    /**
     * @brief Returns the bottom N card IDs without removing them.
     * @param deck_id Target deck.
     * @param count   Number of cards to peek.
     */
    virtual std::vector<CardId> look_bottom_cards(const DeckId& deck_id,
                                                  int count) const = 0;

    /**
     * @brief Marks a specific card as selected (game-defined semantics).
     * @param deck_id Target deck.
     * @param card_id Card to select.
     */
    virtual RuleResult select_specific_card(const DeckId& deck_id,
                                            const CardId& card_id) = 0;

    /**
     * @brief Discards `count` random cards from the given zone.
     * @param deck_id   Target deck.
     * @param zone_name Source zone.
     * @param count     Number of cards to discard.
     */
    virtual RuleResult discard_random_cards(const DeckId& deck_id,
                                            const std::string& zone_name,
                                            int count) = 0;

    /**
     * @brief Places a card on the top of the named deck zone.
     * @param deck_id Target deck.
     * @param card_id Card to place.
     */
    virtual RuleResult place_card_on_top(const DeckId& deck_id,
                                         const CardId& card_id) = 0;

    /**
     * @brief Places a card on the bottom of the named deck zone.
     * @param deck_id Target deck.
     * @param card_id Card to place.
     */
    virtual RuleResult place_card_on_bottom(const DeckId& deck_id,
                                            const CardId& card_id) = 0;

    /**
     * @brief Rolls dice using the given expression and returns the result.
     * @param dice_expression String dice expression, e.g. "2d6", "1d20+3".
     * @return Resolved integer result.
     */
    virtual int roll_dice(const std::string& dice_expression) = 0;

    // ── Events ────────────────────────────────────────────────────────────────

    /**
     * @brief Emits a rule event through the game's event bus.
     *
     * The context implementation may forward this to `gmDispatch`,
     * a custom observer list, or simply log it.
     * @param event Event to emit.
     */
    virtual void emit_event(const RuleEvent& event,
                            const std::string& bus_name = "RuleEvBus") = 0;

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
