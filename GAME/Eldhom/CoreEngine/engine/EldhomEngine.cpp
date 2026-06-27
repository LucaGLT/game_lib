/**
 * @file engine/EldhomEngine.cpp
 * @brief Implementation of EldhomEngine.
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomEngine.hpp"
#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/actors/MonsterGroupState.hpp"
#include "gmActor/core/Enums.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <stdexcept>

namespace eldhom {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

EldhomEngine::EldhomEngine(
	gmActor::ActorStore                                              store,
	std::unordered_map<HeroId, gmAlea::SequenceState>               seq_states,
	std::unordered_map<CardId, EldhomCard>                          card_catalog,
	std::vector<std::string>                                        hero_factions,
	std::vector<std::string>                                        monster_factions,
	std::unordered_map<LocationId, std::vector<LocationId>>         adjacency,
	EldhomBehaviorAdapter                                           behavior_adapter,
	MissionEventSystem                                              event_system,
	EngineEventCallback                                             on_event)
	: _store(std::move(store))
	, _seq_states(std::move(seq_states))
	, _card_catalog(std::move(card_catalog))
	, _hero_factions(std::move(hero_factions))
	, _monster_factions(std::move(monster_factions))
	, _adjacency(std::move(adjacency))
	, _sequence_adapter()
	, _formation_adapter()
	, _targeting()
	, _behavior_adapter(std::move(behavior_adapter))
	, _mission_events(std::move(event_system))
	, _rule_adapter(TargetingFilter{}, _adjacency)   // _adjacency initialized before _rule_adapter
	, _on_event(std::move(on_event))
{
}

// ─────────────────────────────────────────────────────────────────────────────
// from_definition  (static factory)
// ─────────────────────────────────────────────────────────────────────────────

static gmActor::AreaPosition parse_position(const std::string& s)
{
	if (s == "FRONTLINE" || s == "FRONT") { return gmActor::AreaPosition::FRONTLINE; }
	if (s == "BACKLINE"  || s == "BACK")  { return gmActor::AreaPosition::BACKLINE; }
	return gmActor::AreaPosition::NONE;
}

EldhomEngine EldhomEngine::from_definition(
	const MissionDefinition&                                     def,
	const std::unordered_map<CardId, EldhomCard>&                card_catalog,
	const std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>& behavior_catalog,
	EngineEventCallback                                          on_event)
{
	// ── Build adjacency map ───────────────────────────────────────────────────
	std::unordered_map<LocationId, std::vector<LocationId>> adjacency;
	for (const LocationNode& loc : def.locations)
	{
		adjacency[loc.id] = loc.adjacent;
	}

	// ── Populate actor store ──────────────────────────────────────────────────
	gmActor::ActorStore store;
	std::unordered_map<HeroId, gmAlea::SequenceState> seq_states;
	std::vector<std::string> hero_factions;
	std::vector<std::string> monster_factions;

	// PG roster
	for (const PgEntry& pg : def.pg_roster)
	{
		gmActor::HeroState h;
		h.common.actor_id          = pg.hero_id;
		h.common.kind              = gmActor::ActorKind::HERO;
		h.common.display_name      = pg.display_name;
		h.common.faction_id        = pg.faction_id;
		h.common.area_id           = pg.start_location;
		h.common.area_position     = parse_position(pg.start_position);
		h.common.max_hp            = pg.max_hp;
		h.common.current_hp        = pg.max_hp;
		h.common.timeline_position = pg.start_timeline;
		h.common.tie_break_rank    = RANK_HERO;
		h.common.life_state        = gmActor::ActorLifeState::ACTIVE;
		h.level                    = pg.level;
		h.hand_limit               = pg.hand_limit;
		h.mission_deck_id          = pg.hero_id + "_mission_deck";

		store.add_hero(h);
		seq_states[pg.hero_id] = gmAlea::SequenceState{};

		// Collect unique hero factions
		bool faction_known = false;
		for (const std::string& f : hero_factions)
		{
			if (f == pg.faction_id) { faction_known = true; break; }
		}
		if (!faction_known) { hero_factions.push_back(pg.faction_id); }
	}

	// Monster groups + instances
	// Build behavior adapter — use explicit copy to avoid most-vexing-parse
	std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> behavior_cat_copy(
		behavior_catalog);
	EldhomBehaviorAdapter behavior_adapter(std::move(behavior_cat_copy));

	for (const MonsterGroupEntry& entry : def.monster_groups)
	{
		// Group state
		gmActor::MonsterGroupState grp;
		grp.actor_id            = entry.group_id;
		grp.group_id            = entry.group_id;
		grp.monster_type_id     = entry.monster_type;
		grp.display_name        = entry.display_name;
		grp.timeline_position   = entry.start_timeline;
		grp.tie_break_rank      = entry.tie_break_rank;
		grp.enabled             = true;
		grp.removed             = false;

		// Register behavior deck
		BehaviorDeckState deck;
		deck.cards   = entry.behavior_deck;
		deck.current = 0;
		behavior_adapter.register_deck(entry.group_id, deck);

		// Set initial behavior card
		if (!entry.behavior_deck.empty())
		{
			grp.active_behavior_card_id = entry.behavior_deck.front();
		}

		// Monster instances
		for (const MonsterInstanceEntry& inst : entry.instances)
		{
			gmActor::MonsterInstanceState m;
			m.common.actor_id          = inst.instance_id;
			m.common.kind              = gmActor::ActorKind::MONSTER_INSTANCE;
			m.common.display_name      = entry.display_name + " #" + inst.instance_id;
			m.common.faction_id        = entry.faction_id;
			m.common.area_id           = entry.start_location;
			m.common.area_position     = parse_position(inst.position);
			m.common.max_hp            = inst.max_hp;
			m.common.current_hp        = inst.max_hp;
			m.common.life_state        = gmActor::ActorLifeState::ACTIVE;
			m.monster_type_id          = entry.monster_type;
			m.group_id                 = entry.group_id;
			m.base_damage              = inst.damage;
			m.base_movement            = inst.movement;

			store.add_monster_instance(m);
			grp.members.push_back(inst.instance_id);
		}

		store.add_monster_group(grp);

		// Collect unique monster factions
		bool faction_known = false;
		for (const std::string& f : monster_factions)
		{
			if (f == entry.faction_id) { faction_known = true; break; }
		}
		if (!faction_known) { monster_factions.push_back(entry.faction_id); }
	}

	// ── Build mission event system ────────────────────────────────────────────
	// The on_event callback is shared between the engine and the mission system.
	// Use a shared forwarding lambda that references the engine's own callback.
	// Since the engine is not yet constructed here, we capture a shared_ptr to
	// a function wrapper.
	auto shared_on_event =
		std::make_shared<EngineEventCallback>(std::move(on_event));

	MissionEventSystem event_system(
		def,
		[shared_on_event](const EventType& type, const std::string& payload)
		{
			if (*shared_on_event)
			{
				(*shared_on_event)(type, {}, payload);
			}
		});

	EngineEventCallback engine_cb =
		[shared_on_event](const EventType& type,
		                  const std::string& actor_id,
		                  const std::string& payload)
		{
			if (*shared_on_event)
			{
				(*shared_on_event)(type, actor_id, payload);
			}
		};

	EldhomEngine eng(
		std::move(store),
		std::move(seq_states),
		card_catalog,                 // copy — catalog may be shared by tests
		std::move(hero_factions),
		std::move(monster_factions),
		std::move(adjacency),
		std::move(behavior_adapter),
		std::move(event_system),
		std::move(engine_cb));

	eng.build_initial_hands(def.pg_roster);
	return eng;
}

// ─────────────────────────────────────────────────────────────────────────────
// Hand management
// ─────────────────────────────────────────────────────────────────────────────

void EldhomEngine::build_initial_hands(const std::vector<PgEntry>& roster)
{
	std::mt19937 rng(std::random_device{}());

	for (const PgEntry& pg : roster)
	{
		// Skip heroes without a defined mission deck (e.g. test fixtures).
		// play_card() only validates hand membership when a hand state exists.
		if (pg.mission_deck.empty()) { continue; }

		HeroHandState hs;
		hs.deck = pg.mission_deck;
		std::shuffle(hs.deck.begin(), hs.deck.end(), rng);

		const gmActor::HeroState& hero = _store.hero(pg.hero_id);
		int limit = hero.hand_limit;
		int to_draw = std::min(limit, static_cast<int>(hs.deck.size()));
		for (int i = 0; i < to_draw; ++i)
		{
			hs.hand.push_back(hs.deck.back());
			hs.deck.pop_back();
		}
		_hand_states[pg.hero_id] = std::move(hs);
	}
}

void EldhomEngine::draw_to_hand(const HeroId& hero_id)
{
	if (_hand_states.find(hero_id) == _hand_states.end()) { return; }

	const gmActor::HeroState& hero = _store.hero(hero_id);
	int limit = hero.hand_limit;

	HeroHandState& hs = _hand_states.at(hero_id);
	while (static_cast<int>(hs.hand.size()) < limit)
	{
		if (hs.deck.empty())
		{
			if (hs.discard.empty()) { break; }  // truly no cards left

			// Reshuffle discard into deck
			hs.deck = std::move(hs.discard);
			hs.discard.clear();
			std::mt19937 rng(std::random_device{}());
			std::shuffle(hs.deck.begin(), hs.deck.end(), rng);
			emit(EVT_DECK_RESHUFFLED, hero_id, {});
		}
		hs.hand.push_back(hs.deck.back());
		hs.deck.pop_back();
	}
}

const std::vector<CardId>& EldhomEngine::hand_cards(const HeroId& hero_id) const
{
	return _hand_states.at(hero_id).hand;
}

int EldhomEngine::deck_count(const HeroId& hero_id) const
{
	return static_cast<int>(_hand_states.at(hero_id).deck.size());
}

int EldhomEngine::discard_count(const HeroId& hero_id) const
{
	return static_cast<int>(_hand_states.at(hero_id).discard.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Emit
// ─────────────────────────────────────────────────────────────────────────────

void EldhomEngine::emit(
	const EventType&   type,
	const std::string& actor_id,
	const std::string& payload) const
{
	if (_on_event) { _on_event(type, actor_id, payload); }
}

// ─────────────────────────────────────────────────────────────────────────────
// next_actor / next_actor_kind
// ─────────────────────────────────────────────────────────────────────────────

std::string EldhomEngine::next_actor() const
{
	// Priority 1: any hero with an active non-interrupted sequence.
	// During a sequence the acting hero must complete it before anyone else acts.
	for (const auto& kv : _seq_states)
	{
		if (!kv.second.active || kv.second.interrupted) { continue; }

		if (!_store.has_actor(kv.first)) { continue; }
		const gmActor::ActorStateCommon& c = _store.common(kv.first);
		if (c.removed)                                          { continue; }
		if (c.life_state != gmActor::ActorLifeState::ACTIVE)   { continue; }

		return kv.first;  // this hero keeps priority until the sequence closes
	}

	// Priority 2: normal timeline selection (lowest position + tie-break rank).
	const std::vector<gmActor::ActorId>& ids = _store.timeline_actor_ids();

	std::string best_id;
	int         best_pos  = std::numeric_limits<int>::max();
	int         best_rank = std::numeric_limits<int>::max();

	for (const gmActor::ActorId& id : ids)
	{
		gmActor::ActorKind kind = _store.actor_kind(id);
		int pos  = 0;
		int rank = 0;

		if (kind == gmActor::ActorKind::MONSTER_GROUP)
		{
			const gmActor::MonsterGroupState& g = _store.monster_group(id);
			if (g.removed) { continue; }
			pos  = g.timeline_position;
			rank = g.tie_break_rank;
		}
		else
		{
			const gmActor::ActorStateCommon& c = _store.common(id);
			if (c.removed)  { continue; }
			if (!c.can_act) { continue; }
			if (c.life_state == gmActor::ActorLifeState::KO ||
			    c.life_state == gmActor::ActorLifeState::DEAD ||
			    c.life_state == gmActor::ActorLifeState::REMOVED)
			{
				continue;
			}
			pos  = c.timeline_position;
			rank = c.tie_break_rank;
		}

		if (pos < best_pos || (pos == best_pos && rank < best_rank))
		{
			best_pos  = pos;
			best_rank = rank;
			best_id   = id;
		}
	}

	return best_id;
}

gmActor::ActorKind EldhomEngine::next_actor_kind() const
{
	const std::string& id = next_actor();
	if (id.empty()) { return gmActor::ActorKind::MISSION_SYSTEM; }
	return _store.actor_kind(id);
}

// ─────────────────────────────────────────────────────────────────────────────
// PG turn — do_simple_action
// ─────────────────────────────────────────────────────────────────────────────

ActionResult EldhomEngine::do_simple_action(
	const HeroId&     hero_id,
	SimpleActionType  action_type,
	const LocationId& destination)
{
	if (_mission_events.is_over()) { return { ActionResultCode::OK, "Mission over" }; }

	// Validate it is this hero's turn
	if (next_actor() != hero_id)
	{
		return { ActionResultCode::ERR_NOT_YOUR_TURN,
		         "Not " + hero_id + "'s turn" };
	}

	if (!_store.has_actor(hero_id))
	{
		return { ActionResultCode::ERR_UNKNOWN_ACTOR, hero_id };
	}

	const gmActor::ActorStateCommon& c = _store.common(hero_id);
	if (c.life_state == gmActor::ActorLifeState::KO ||
	    c.life_state == gmActor::ActorLifeState::DEAD)
	{
		return { ActionResultCode::ERR_ACTOR_KO, hero_id + " is KO" };
	}

	EffectResult eff;
	int          cost = 0;

	switch (action_type)
	{
	case SimpleActionType::MOVE:
		cost = COST_SIMPLE_MOVE;
		eff  = _rule_adapter.apply_simple_move(hero_id, destination, _store);
		if (!eff.resolved)
		{
			return { ActionResultCode::ERR_NO_VALID_TARGET,
			         "MOVE failed: " + eff.note };
		}
		emit(EVT_PG_MOVED, hero_id, destination);
		break;

	case SimpleActionType::ATTACK:
	{
		cost = COST_SIMPLE_ATTACK;
		const std::string& faction = enemy_faction_for_hero(hero_id);
		if (faction.empty())
		{
			return { ActionResultCode::ERR_NO_VALID_TARGET,
			         hero_id + ": no enemy in location" };
		}
		eff = _rule_adapter.apply_simple_attack(hero_id, _store, faction);
		if (!eff.resolved)
		{
			return { ActionResultCode::ERR_NO_VALID_TARGET, eff.note };
		}
		emit(EVT_PG_ATTACKED, hero_id, eff.target_id);
		if (eff.target_ko)
		{
			emit(EVT_MONSTER_DEFEATED, eff.target_id, {});
			handle_monster_instance_death(eff.target_id);
		}
		break;
	}

	case SimpleActionType::INTERACT:
		cost = COST_SIMPLE_INTERACT;
		eff.resolved = true;
		eff.note     = hero_id + " interacted";
		break;

	case SimpleActionType::RECOVER:
		cost = COST_SIMPLE_RECOVER;
		eff  = _rule_adapter.apply_simple_recover(hero_id, _store);
		emit(EVT_PG_HEALED, hero_id, std::to_string(eff.hp_restored));
		break;
	}

	// Advance hero timeline
	_store.common(hero_id).timeline_position += cost;
	_mission_events.advance_time(cost);
	emit(EVT_MISSION_TIME, {}, std::to_string(_mission_events.mission_time()));

	// Formation check in the hero's current location
	check_formation(_store.common(hero_id).area_id);

	emit(EVT_PG_SIMPLE_ACTION, hero_id,
		 std::to_string(static_cast<int>(action_type)));

	end_hero_turn(hero_id);
	return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// PG turn — play_card
// ─────────────────────────────────────────────────────────────────────────────

ActionResult EldhomEngine::play_card(const HeroId& hero_id, const CardId& card_id)
{
	if (_mission_events.is_over()) { return { ActionResultCode::OK, "Mission over" }; }

	if (next_actor() != hero_id)
	{
		return { ActionResultCode::ERR_NOT_YOUR_TURN, hero_id };
	}

	if (!_store.has_actor(hero_id))
	{
		return { ActionResultCode::ERR_UNKNOWN_ACTOR, hero_id };
	}

	const gmActor::ActorStateCommon& c = _store.common(hero_id);
	if (c.life_state == gmActor::ActorLifeState::KO ||
	    c.life_state == gmActor::ActorLifeState::DEAD)
	{
		return { ActionResultCode::ERR_ACTOR_KO, hero_id + " is KO" };
	}

	// Look up card
	auto card_it = _card_catalog.find(card_id);
	if (card_it == _card_catalog.end())
	{
		return { ActionResultCode::ERR_CARD_NOT_IN_HAND,
		         "Card not in catalog: " + card_id };
	}
	const EldhomCard& card = card_it->second;

	// Validate card is in the hero's hand (if hand tracking is active)
	auto hand_it = _hand_states.find(hero_id);
	if (hand_it != _hand_states.end())
	{
		const std::vector<CardId>& h = hand_it->second.hand;
		auto pos = std::find(h.begin(), h.end(), card_id);
		if (pos == h.end())
		{
			return { ActionResultCode::ERR_CARD_NOT_IN_HAND,
			         "Card not in hand: " + card_id };
		}
	}

	// Sequence check
	const gmAlea::SequenceState& seq = _seq_states.at(hero_id);
	if (!_sequence_adapter.can_play(card.card_type, seq))
	{
		return { ActionResultCode::ERR_CARD_NOT_PLAYABLE,
		         "Sequence engine rejected: " + card_id };
	}

	// Determine enemy faction for targeting
	const std::string& enemy_faction = enemy_faction_for_hero(hero_id);

	// Apply effects
	for (const EldhomEffect& eff : card.effects)
	{
		EffectResult res = _rule_adapter.apply_effect(eff, hero_id, _store, enemy_faction);
		if (res.target_ko && !res.target_id.empty())
		{
			emit(EVT_MONSTER_DEFEATED, res.target_id, {});
			handle_monster_instance_death(res.target_id);
		}
	}

	// Save original state for is_turn_ending check BEFORE advancing
	gmAlea::CardType original_card_type = card.card_type;
	gmAlea::SequenceState seq_before = seq; // copy (value semantics)

	// Advance sequence state
	_seq_states[hero_id] = _sequence_adapter.advance(original_card_type, seq_before);

	// Advance timeline
	_store.common(hero_id).timeline_position += card.timeline_cost;
	_mission_events.advance_time(card.timeline_cost);

	// Move played card from hand to discard
	if (_hand_states.count(hero_id))
	{
		HeroHandState& hs = _hand_states.at(hero_id);
		auto pos = std::find(hs.hand.begin(), hs.hand.end(), card_id);
		if (pos != hs.hand.end())
		{
			hs.discard.push_back(*pos);
			hs.hand.erase(pos);
		}
	}

	emit(EVT_PG_PLAYED_CARD, hero_id, card_id);

	// Formation check
	check_formation(_store.common(hero_id).area_id);

	// Turn-ending check uses the state BEFORE the advance
	if (_sequence_adapter.is_turn_ending(original_card_type, seq_before))
	{
		end_hero_turn(hero_id);
	}

	return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// PG turn — stop_sequence
// ─────────────────────────────────────────────────────────────────────────────

ActionResult EldhomEngine::stop_sequence(const HeroId& hero_id)
{
	if (next_actor() != hero_id)
	{
		return { ActionResultCode::ERR_NOT_YOUR_TURN, hero_id };
	}

	const gmAlea::SequenceState& seq = _seq_states.at(hero_id);
	if (!seq.active)
	{
		return { ActionResultCode::ERR_NO_SEQUENCE_ACTIVE, hero_id };
	}

	_seq_states[hero_id] = _sequence_adapter.reset();
	emit(EVT_SEQUENCE_ENDED, hero_id, "voluntary_stop");
	end_hero_turn(hero_id);
	return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// Monster group turn
// ─────────────────────────────────────────────────────────────────────────────

ActionResult EldhomEngine::resolve_next_group_turn()
{
	if (next_actor_kind() != gmActor::ActorKind::MONSTER_GROUP)
	{
		return { ActionResultCode::ERR_NOT_YOUR_TURN,
		         "Next actor is not a monster group" };
	}
	return resolve_group_turn_for(next_actor());
}

ActionResult EldhomEngine::resolve_group_turn_for(const GroupId& group_id)
{
	if (_mission_events.is_over()) { return {}; }

	if (!_store.has_actor(group_id))
	{
		return { ActionResultCode::ERR_UNKNOWN_ACTOR, group_id };
	}

	gmActor::MonsterGroupState& group = _store.monster_group(group_id);
	if (group.removed) { return {}; }

	// Determine the first non-empty hero faction location for targeting
	const std::string& hero_faction =
		_hero_factions.empty() ? std::string{"HEROES"} : _hero_factions.front();

	emit(EVT_GROUP_ACTIVATED, group_id, {});

	// Build the step executor lambda.
	// Captures only pointers/references that outlive this call.
	EldhomRuleAdapter&   ra     = _rule_adapter;
	gmActor::ActorStore& st     = _store;
	EldhomEngine*        engine = this;

	gmActor::StepExecutor executor =
		[&ra, &st, &hero_faction, engine]
		(const gmActor::ActorId& grp_id,
		 const gmActor::ActorId& member_id,
		 const gmActor::BehaviorStep& step) -> bool
		{
			// Skip already-dead members
			if (!st.has_actor(member_id)) { return false; }
			const gmActor::ActorStateCommon& mc = st.common(member_id);
			if (mc.life_state == gmActor::ActorLifeState::DEAD)   { return false; }
			if (mc.life_state == gmActor::ActorLifeState::REMOVED) { return false; }

			EffectResult res =
				ra.apply_behavior_step(step, grp_id, member_id, st, hero_faction);

			if (res.target_ko && !res.target_id.empty())
			{
				engine->emit(EVT_PG_KO, res.target_id, grp_id);
				engine->_mission_events.notify_pg_ko(engine->active_pg_count());
			}

			return res.resolved;
		};

	// Execute the behavior card
	_behavior_adapter.process_group_turn(group, _store, executor);

	// Advance behavior deck
	_behavior_adapter.advance_behavior_card(group_id);

	// Formation check in the group's member locations
	for (const gmActor::ActorId& member_id : group.members)
	{
		if (_store.has_actor(member_id))
		{
			const gmActor::ActorStateCommon& mc = _store.common(member_id);
			if (!mc.removed) { check_formation(mc.area_id); }
		}
	}

	emit(EVT_GROUP_ACTIVATED, group_id,
	     "timeline=" + std::to_string(group.timeline_position));

	return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// State queries
// ─────────────────────────────────────────────────────────────────────────────

int EldhomEngine::mission_time() const
{
	return _mission_events.mission_time();
}

MissionOutcome EldhomEngine::mission_outcome() const
{
	return _mission_events.outcome();
}

bool EldhomEngine::is_over() const
{
	return _mission_events.is_over();
}

const gmAlea::SequenceState& EldhomEngine::sequence_state(const HeroId& hero_id) const
{
	return _seq_states.at(hero_id);
}

int EldhomEngine::timeline_position(const std::string& actor_id) const
{
	if (!_store.has_actor(actor_id))
	{
		throw std::out_of_range("EldhomEngine::timeline_position: unknown actor " + actor_id);
	}
	gmActor::ActorKind kind = _store.actor_kind(actor_id);
	if (kind == gmActor::ActorKind::MONSTER_GROUP)
	{
		return _store.monster_group(actor_id).timeline_position;
	}
	return _store.common(actor_id).timeline_position;
}

const gmActor::ActorStore& EldhomEngine::actor_store() const
{
	return _store;
}

void EldhomEngine::set_event_callback(EngineEventCallback cb)
{
	_on_event = std::move(cb);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void EldhomEngine::check_formation(const LocationId& location_id)
{
	// Check all known factions in this location
	std::vector<std::string> all_factions;
	for (const std::string& f : _hero_factions)    { all_factions.push_back(f); }
	for (const std::string& f : _monster_factions) { all_factions.push_back(f); }

	for (const std::string& faction : all_factions)
	{
		FormationCheckResult res =
			_formation_adapter.check(_store, location_id, faction);

		if (res.scompaginamento)
		{
			_formation_adapter.resolve_scompaginamento(_store, location_id, faction);
			emit(EVT_FORMATION_CHANGED, {},
			     faction + "@" + location_id + ":scompaginamento");
		}
		else if (!res.valid)
		{
			emit(EVT_FORMATION_CHECKED, {},
			     faction + "@" + location_id + ":overflow=" +
			     std::to_string(res.overflow));
		}
	}
}

int EldhomEngine::active_group_count() const
{
	int count = 0;
	for (const auto& kv : _store.monster_groups())
	{
		if (!kv.second.removed) { ++count; }
	}
	return count;
}

int EldhomEngine::active_pg_count() const
{
	int count = 0;
	for (const auto& kv : _store.heroes())
	{
		if (kv.second.common.life_state == gmActor::ActorLifeState::ACTIVE) { ++count; }
	}
	return count;
}

void EldhomEngine::end_hero_turn(const HeroId& hero_id)
{
	// Reset sequence if it was not already closed by a SINGLE/SEQ_END card
	const gmAlea::SequenceState& seq = _seq_states.at(hero_id);
	if (seq.active)
	{
		_seq_states[hero_id] = _sequence_adapter.reset();
	}

	// Draw back up to hand limit and emit hand update
	if (_hand_states.count(hero_id))
	{
		draw_to_hand(hero_id);

		const std::vector<CardId>& hand = _hand_states.at(hero_id).hand;
		std::string payload = "[";
		for (std::size_t i = 0; i < hand.size(); ++i)
		{
			if (i > 0) { payload += ","; }
			payload += "\"" + hand[i] + "\"";
		}
		payload += "]";
		emit(EVT_HAND_CHANGED, hero_id, payload);
	}

	emit(EVT_PG_TURN_ENDED, hero_id, {});
}

std::string EldhomEngine::enemy_faction_for_hero(const HeroId& hero_id) const
{
	const std::string& loc = _store.hero(hero_id).common.area_id;
	for (const std::string& faction : _monster_factions)
	{
		if (_targeting.has_valid_target(_store, loc, faction)) { return faction; }
	}
	return {};
}

void EldhomEngine::handle_monster_instance_death(const gmActor::ActorId& instance_id)
{
	// Mark instance as removed
	gmActor::MonsterInstanceState& m = _store.monster_instance(instance_id);
	m.common.removed         = true;
	m.common.can_be_targeted = false;
	m.common.life_state      = gmActor::ActorLifeState::DEAD;

	// Find the owning group and remove the instance from its member list
	const std::string& group_id = m.group_id;
	if (_store.has_actor(group_id))
	{
		gmActor::MonsterGroupState& grp = _store.monster_group(group_id);
		grp.members.erase(
			std::remove(grp.members.begin(), grp.members.end(), instance_id),
			grp.members.end());

		// If the group has no more members, remove the group
		if (grp.members.empty())
		{
			grp.removed = true;
			emit(EVT_GROUP_ELIMINATED, group_id, {});
			_mission_events.notify_group_eliminated(active_group_count());
		}
	}

	emit(EVT_MONSTER_DAMAGED, instance_id, "killed");
}

} // namespace eldhom
