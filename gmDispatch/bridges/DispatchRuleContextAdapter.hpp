#ifndef GMDISPATCH_DISPATCHRULECONTEXTADAPTER_HPP
#define GMDISPATCH_DISPATCHRULECONTEXTADAPTER_HPP

/**
 * @file bridges/DispatchRuleContextAdapter.hpp
 * @brief RuleContext adapter that publishes gmRules events to gmDispatch.
 */

#include "RuleEventBridge.hpp"

#include "gmRules/core/RuleContext.hpp"

#include <string>
#include <vector>

namespace gmDispatch {

/**
 * @brief Wraps a RuleContext and dispatches emitted rule events to gmDispatch.
 *
 * The wrapped context remains the source of truth for all game-state
 * queries/mutations. Event publication is handled by this adapter.
 */
class DispatchRuleContextAdapter : public gmRules::RuleContext
{
public:
	DispatchRuleContextAdapter(gmRules::RuleContext& inner,
						   GmDispatcher& bus);

	bool has_actor(const gmRules::ActorId& actor_id) const override;
	bool actor_has_tag(const gmRules::ActorId& actor_id,
				   const std::string& tag) const override;
	int actor_current_hp(const gmRules::ActorId& actor_id) const override;
	int actor_max_hp(const gmRules::ActorId& actor_id) const override;
	bool actor_has_status(const gmRules::ActorId& actor_id,
				  const gmRules::StatusId& status_id) const override;
	std::vector<gmRules::StatusInstanceId>
	statuses_on_actor(const gmRules::ActorId& actor_id) const override;
	bool are_allies(const gmRules::ActorId& a,
			 const gmRules::ActorId& b) const override;
	bool are_enemies(const gmRules::ActorId& a,
			  const gmRules::ActorId& b) const override;
	int actor_resource(const gmRules::ActorId& actor_id,
				   const std::string& resource_id) const override;

	void modify_actor_hp(const gmRules::ActorId& actor_id,
				 int delta) override;
	void add_actor_tag(const gmRules::ActorId& actor_id,
			  const std::string& tag) override;
	void remove_actor_tag(const gmRules::ActorId& actor_id,
				 const std::string& tag) override;

	// Chapter 4 — gmActor lifecycle
	void spawn_actor(const gmRules::ActorId& actor_id,
				 const std::string& spec_json) override;
	void despawn_actor(const gmRules::ActorId& actor_id) override;
	void revive_actor(const gmRules::ActorId& actor_id) override;
	void change_actor_team(const gmRules::ActorId& actor_id,
				   const std::string& team_id) override;

	// Chapter 4 — gmActor resources / equipment
	void modify_resource(const gmRules::ActorId& actor_id,
				 const std::string& resource_id,
				 int delta) override;
	void set_resource_max(const gmRules::ActorId& actor_id,
				  const std::string& resource_id,
				  int max_value) override;
	void equip_item(const gmRules::ActorId& actor_id,
			const std::string& item_id) override;
	void unequip_item(const gmRules::ActorId& actor_id,
			  const std::string& slot_id) override;

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

	// Chapter 6 — gmMap queries
	bool is_location_reachable(const gmRules::LocationId& from,
					const gmRules::LocationId& to) const override;
	bool has_line_of_sight(const gmRules::LocationId& from,
				   const gmRules::LocationId& to) const override;
	int move_cost_between(const gmRules::LocationId& from,
				  const gmRules::LocationId& to) const override;

	void move_actor_to_location(const gmRules::ActorId& actor_id,
					 const gmRules::LocationId& location_id) override;

	// Chapter 6 — gmMap mutations
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

	// Chapter 5 — gmAlea
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

private:
	gmRules::RuleContext& _inner;
	RuleEventBridge _bridge;
};

} // namespace gmDispatch

#endif // GMDISPATCH_DISPATCHRULECONTEXTADAPTER_HPP
