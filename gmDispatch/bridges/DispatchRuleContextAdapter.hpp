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

	void modify_actor_hp(const gmRules::ActorId& actor_id,
				 int delta) override;
	void add_actor_tag(const gmRules::ActorId& actor_id,
			  const std::string& tag) override;
	void remove_actor_tag(const gmRules::ActorId& actor_id,
				 const std::string& tag) override;

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

	void move_actor_to_location(const gmRules::ActorId& actor_id,
					 const gmRules::LocationId& location_id) override;

	bool has_deck(const gmRules::DeckId& deck_id) const override;
	std::vector<gmRules::CardId> draw_cards(const gmRules::DeckId& deck_id,
						 int amount) override;
	gmRules::RuleResult move_card_to_zone(const gmRules::DeckId& deck_id,
						 const gmRules::CardId& card_id,
						 const std::string& zone_name) override;

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
