#ifndef GMDUNGEONBASIC_DUNGEONRULEADAPTER_HPP
#define GMDUNGEONBASIC_DUNGEONRULEADAPTER_HPP

/**
 * @file rules/DungeonRuleAdapter.hpp
 * @brief Bridge between the dungeon game state and the gmRules rule engine.
 *
 * DungeonRuleAdapter implements @c gmRules::RuleContext for the dungeon domain.
 * It exposes the current map and actor state to gmRules so that
 * ConditionSpec / EffectSpec evaluations defined in the GRS specifications can
 * query and mutate the game world through the standard RuleContext interface.
 *
 * Additionally, it provides direct convenience query methods (can_move, can_heal,
 * can_equip) used by @ref ActionV1 to validate actions before execution.
 *
 * @note All methods that mutate state must be called from the engine thread.
 */

#include "actors/ActorRoster.hpp"
#include "engine/DungeonTypes.hpp"
#include "world/DungeonMap.hpp"

#include "gmRules/core/RuleContext.hpp"
#include "gmRules/facade/gmRulesEngine.hpp"

#include <string>
#include <vector>

namespace gmDungeonBasic
{

/**
 * @brief Rule context and convenience validator for v1 dungeon actions.
 *
 * Holds non-owning references to the DungeonMap and ActorRoster so that rule
 * conditions can query game state without owning it. The engine constructs this
 * adapter after loading the map and populating the roster, then passes it to
 * ActionV1 for action validation.
 */
class DungeonRuleAdapter : public gmRules::RuleContext
{
public:
	/**
	 * @brief Constructs the adapter with references to the live game state.
	 *
	 * @param map     Reference to the dungeon map (must outlive this adapter).
	 * @param actors  Reference to the actor roster (must outlive this adapter).
	 */
	DungeonRuleAdapter(DungeonMap& map, ActorRoster& actors);

	// ── v1 action condition checks ────────────────────────────────────────────

	/**
	 * @brief Checks whether the hero can legally move to a destination room.
	 *
	 * Evaluates conditions: C_HeroExists AND C_DestinationValid AND the
	 * destination must be adjacent to the hero's current location. Stunned heroes
	 * are blocked by trigger T_BlockIfStunned.
	 *
	 * @param hero_id      Hero actor identifier.
	 * @param destination  Target room identifier.
	 * @return             @c true if the move is allowed by the current rule set.
	 */
	bool can_move(const std::string& hero_id, const std::string& destination) const;

	/**
	 * @brief Checks whether the hero can use a healing potion.
	 *
	 * Evaluates condition C_HeroCanHeal: ACTOR_HAS_TAG(hero_id, has_potion).
	 *
	 * @param hero_id    Hero actor identifier.
	 * @param target_id  Target actor identifier (may equal hero_id for self-heal).
	 * @return           @c true if the heal action is currently valid.
	 */
	bool can_heal(const std::string& hero_id, const std::string& target_id) const;

	/**
	 * @brief Checks whether the hero can equip a weapon.
	 *
	 * Evaluates conditions C_HasBigSword AND C_NoWeaponEquipped.
	 *
	 * @param hero_id   Hero actor identifier.
	 * @param item_tag  Item tag to equip (e.g. "bigword_available").
	 * @return          @c true if the equip action is currently valid.
	 */
	bool can_equip(const std::string& hero_id, const std::string& item_tag) const;

	// ── Rejection reason ─────────────────────────────────────────────────────

	/**
	 * @brief Returns a human-readable reason why the last condition check failed.
	 *
	 * Valid only immediately after a @c can_*() call that returned @c false.
	 *
	 * @return  Rejection reason string (used in ACTION_REJECTED event).
	 */
	std::string rejection_reason() const;

	// ── Card rule execution ───────────────────────────────────────────────────

	/**
	 * @brief Loads card rule definitions from a JSON file into the rules engine.
	 *
	 * The JSON file must follow the @c RuleBookLoader format (array @c "rules",
	 * each with @c rule_id, @c description, @c preconditions[], @c effects[]).
	 *
	 * @param path  Absolute or working-directory-relative path to the JSON file.
	 * @return      @c true if the file was loaded and parsed without errors.
	 */
	bool load_card_rules(const std::string& path);

	/**
	 * @brief Executes a card's rule group: evaluates preconditions and applies effects.
	 *
	 * Looks up the card_id in the rule book, evaluates all preconditions against
	 * the current game state, and if valid applies all effects via the rule engine.
	 * If a target_id is provided it is used for SELECTED_ENEMY effects; otherwise
	 * the first living enemy in the same room as the hero is auto-selected.
	 *
	 * @param hero_id          Actor playing the card.
	 * @param card_id          Identifier of the card being played.
	 * @param target_id        Optional explicit target actor id.
	 * @param out_rejection    On failure, filled with the rejection reason.
	 * @return                 @c true if all effects were applied, @c false if rejected.
	 */
	bool execute_card(const std::string& hero_id,
	                  const std::string& card_id,
	                  const std::string& target_id,
	                  std::string& out_rejection);

private:
	// ── gmRules::RuleContext implementation ──────────────────────────────────

	bool has_actor(const gmRules::ActorId& actor_id) const override;
	bool actor_has_tag(const gmRules::ActorId& actor_id,
	                   const std::string& tag) const override;
	int actor_current_hp(const gmRules::ActorId& actor_id) const override;
	int actor_max_hp(const gmRules::ActorId& actor_id) const override;
	bool actor_has_status(const gmRules::ActorId& actor_id,
	                      const gmRules::StatusId& status_id) const override;
	std::vector<gmRules::StatusInstanceId>
	statuses_on_actor(const gmRules::ActorId& actor_id) const override;
	bool are_allies(const gmRules::ActorId& a, const gmRules::ActorId& b) const override;
	bool are_enemies(const gmRules::ActorId& a, const gmRules::ActorId& b) const override;
	int actor_resource(const gmRules::ActorId& actor_id,
	                  const std::string& resource_id) const override;

	void modify_actor_hp(const gmRules::ActorId& actor_id, int delta) override;
	void add_actor_tag(const gmRules::ActorId& actor_id, const std::string& tag) override;
	void remove_actor_tag(const gmRules::ActorId& actor_id,
	                     const std::string& tag) override;
	void spawn_actor(const gmRules::ActorId& actor_id,
	                const std::string& spec_json) override;
	void despawn_actor(const gmRules::ActorId& actor_id) override;
	void revive_actor(const gmRules::ActorId& actor_id) override;
	void change_actor_team(const gmRules::ActorId& actor_id,
	                      const std::string& team_id) override;
	void modify_resource(const gmRules::ActorId& actor_id,
	                    const std::string& resource_id,
	                    int delta) override;
	void set_resource_max(const gmRules::ActorId& actor_id,
	                     const std::string& resource_id,
	                     int max_value) override;
	void equip_item(const gmRules::ActorId& actor_id, const std::string& item_id) override;
	void unequip_item(const gmRules::ActorId& actor_id, const std::string& slot_id) override;

	void add_status_instance(const gmRules::StatusInstance& status) override;
	void remove_status_instance(const gmRules::StatusInstanceId& instance_id) override;

	bool has_location(const gmRules::LocationId& location_id) const override;
	gmRules::LocationId actor_location(const gmRules::ActorId& actor_id) const override;
	bool are_locations_adjacent(const gmRules::LocationId& a,
	                            const gmRules::LocationId& b) const override;
	int distance_between_locations(const gmRules::LocationId& a,
	                               const gmRules::LocationId& b) const override;
	bool location_has_tag(const gmRules::LocationId& location_id,
	                     const std::string& tag) const override;
	std::vector<gmRules::ActorId>
	actors_in_location(const gmRules::LocationId& location_id) const override;

	bool is_location_reachable(const gmRules::LocationId& from,
	                          const gmRules::LocationId& to) const override;
	bool has_line_of_sight(const gmRules::LocationId& from,
	                      const gmRules::LocationId& to) const override;
	int move_cost_between(const gmRules::LocationId& from,
	                     const gmRules::LocationId& to) const override;
	void move_actor_to_location(const gmRules::ActorId& actor_id,
	                            const gmRules::LocationId& location_id) override;
	void set_location_passable(const gmRules::LocationId& location_id,
	                          bool passable) override;
	void add_location_tag(const gmRules::LocationId& location_id,
	                     const std::string& tag) override;
	void remove_location_tag(const gmRules::LocationId& location_id,
	                        const std::string& tag) override;
	void set_location_owner(const gmRules::LocationId& location_id,
	                       const std::string& owner_id) override;
	void create_barrier(const gmRules::LocationId& from,
	                   const gmRules::LocationId& to,
	                   const std::string& barrier_id) override;
	void remove_barrier(const std::string& barrier_id) override;
	void spawn_interactable(const gmRules::LocationId& location_id,
	                       const std::string& spec_json) override;
	void despawn_interactable(const std::string& interactable_id) override;

	bool has_deck(const gmRules::DeckId& deck_id) const override;
	std::vector<gmRules::CardId> draw_cards(const gmRules::DeckId& deck_id,
	                                        int amount) override;
	gmRules::RuleResult move_card_to_zone(const gmRules::DeckId& deck_id,
	                                      const gmRules::CardId& card_id,
	                                      const std::string& zone_name) override;
	int deck_zone_count(const gmRules::DeckId& deck_id,
	                   const std::string& zone_name) const override;
	bool card_in_zone(const gmRules::DeckId& deck_id,
	                 const gmRules::CardId& card_id,
	                 const std::string& zone_name) const override;
	void shuffle_zone(const gmRules::DeckId& deck_id,
	                 const std::string& zone_name) override;
	std::vector<gmRules::CardId> look_top_cards(const gmRules::DeckId& deck_id,
	                                            int count) const override;
	std::vector<gmRules::CardId> look_bottom_cards(const gmRules::DeckId& deck_id,
	                                               int count) const override;
	gmRules::RuleResult select_specific_card(const gmRules::DeckId& deck_id,
	                                         const gmRules::CardId& card_id) override;
	gmRules::RuleResult discard_random_cards(const gmRules::DeckId& deck_id,
	                                         const std::string& zone_name,
	                                         int count) override;
	gmRules::RuleResult place_card_on_top(const gmRules::DeckId& deck_id,
	                                      const gmRules::CardId& card_id) override;
	gmRules::RuleResult place_card_on_bottom(const gmRules::DeckId& deck_id,
	                                         const gmRules::CardId& card_id) override;
	int roll_dice(const std::string& dice_expression) override;

	void emit_event(const gmRules::RuleEvent& event,
	               const std::string& bus_name = "RuleEvBus") override;
	gmRules::RuleResult apply_extended_effect(const gmRules::EffectSpec& effect,
	                                          const gmRules::TargetRef& target,
	                                          const gmRules::ActorId& source_actor_id,
	                                          gmRules::RuleEvent* out_event) override;

	bool is_hero_actor(const std::string& actor_id) const;

	DungeonMap&  _map;     ///< Non-owning reference to the dungeon map.
	ActorRoster& _actors;  ///< Non-owning reference to the actor roster.

	mutable std::string _rejection_reason;  ///< Set by failed can_*() calls.
	mutable gmRules::gmRulesEngine _rules_engine;
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_DUNGEONRULEADAPTER_HPP
