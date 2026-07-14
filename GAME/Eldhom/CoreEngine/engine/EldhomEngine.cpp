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

namespace {

// Builds the compact JSON payload for EVT_ZONE_DOOR_OPENED (location IDs are
// plain ASCII, so no escaping is needed here).
std::string zone_door_payload(const std::pair<LocationId, LocationId>& door)
{
	return std::string("{\"a\":\"") + door.first + "\",\"b\":\"" + door.second + "\"}";
}

} // namespace

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
			const LocationId spawn_loc =
				inst.start_location.empty() ? entry.start_location : inst.start_location;
			m.common.actor_id          = inst.instance_id;
			m.common.kind              = gmActor::ActorKind::MONSTER_INSTANCE;
			m.common.display_name      = entry.display_name + " #" + inst.instance_id;
			m.common.faction_id        = entry.faction_id;
			m.common.area_id           = spawn_loc;
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

	// ── Populate special objects ──────────────────────────────────────────────
	eng._special_objects = def.special_objects;
	for (const SpecialObject& obj : def.special_objects)
	{
		eng._special_object_used[obj.object_id] = false;
	}

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
		if (pg.mission_deck.empty()) { continue; }

		HeroHandState hs;
		// Rule: all cards start in the discard pile, then are shuffled into
		// the draw pile, then hand_limit cards are dealt to the hand.
		hs.discard = pg.mission_deck;
		hs.deck    = std::move(hs.discard);
		hs.discard.clear();
		std::shuffle(hs.deck.begin(), hs.deck.end(), rng);

		// Store state first so emit() can read it via _hand_states.
		_hand_states[pg.hero_id] = std::move(hs);
		emit(EVT_DECK_RESHUFFLED, pg.hero_id, {});

		const gmActor::HeroState& hero = _store.hero(pg.hero_id);
		int limit   = hero.hand_limit;
		HeroHandState& hsr = _hand_states[pg.hero_id];
		int to_draw = std::min(limit, static_cast<int>(hsr.deck.size()));
		for (int i = 0; i < to_draw; ++i)
		{
			hsr.hand.push_back(hsr.deck.back());
			hsr.deck.pop_back();
		}

		// Build hand payload and notify.
		std::string payload = "[";
		for (std::size_t i = 0; i < hsr.hand.size(); ++i)
		{
			if (i > 0) { payload += ","; }
			payload += "\"" + hsr.hand[i] + "\"";
		}
		payload += "]";
		emit(EVT_HAND_CHANGED, pg.hero_id, payload);
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

const std::vector<CardId>& EldhomEngine::discard_cards(const HeroId& hero_id) const
{
	return _hand_states.at(hero_id).discard;
}

int EldhomEngine::played_count(const HeroId& hero_id) const
{
	return static_cast<int>(_hand_states.at(hero_id).played.size());
}

const std::vector<CardId>& EldhomEngine::played_cards(const HeroId& hero_id) const
{
	return _hand_states.at(hero_id).played;
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
	const HeroId&              hero_id,
	SimpleActionType           action_type,
	const LocationId&          destination,
	const std::vector<CardId>& discard_ids)
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
		eff  = _rule_adapter.apply_card_move(hero_id, destination, 2, _store);
		if (!eff.resolved)
		{
			return { ActionResultCode::ERR_NO_VALID_TARGET,
			         "MOVE failed: " + eff.note };
		}
		// Crossing a still-closed zone-boundary door costs +1 extra timeline cost and opens
		// it permanently (PGs and monsters can freely cross it from now on).
		cost += eff.extra_timeline_cost;
		for (const std::pair<LocationId, LocationId>& door : eff.opened_doors)
		{
			emit(EVT_ZONE_DOOR_OPENED, hero_id, zone_door_payload(door));
		}
		emit(EVT_PG_MOVED, hero_id, destination);
		check_pg_at_location(hero_id, destination);
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
		{
			std::string atk_payload =
				std::string("{\"target\":\"") + eff.target_id
				+ "\",\"damage\":" + std::to_string(eff.damage_dealt)
				+ ",\"type\":\"MELEE\",\"range\":0}";
			emit(EVT_PG_ATTACKED, hero_id, atk_payload);
		}
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
		trigger_special_object(hero_id);
		break;

	case SimpleActionType::RECOVER:
		cost = COST_SIMPLE_RECOVER;
		eff  = _rule_adapter.apply_simple_recover(hero_id, _store);
		emit(EVT_PG_HEALED, hero_id, std::to_string(eff.hp_restored));
		{
			// Scarta fino a N carte e pescane altrettante: N=1 normale, N=2 se
			// l'eroe è in Retroguardia (Riprendere Fiato).
			const int max_discard =
				(c.area_position == gmActor::AreaPosition::BACKLINE) ? 2 : 1;
			int discarded = 0;
			for (const CardId& cid : discard_ids)
			{
				if (discarded >= max_discard) { break; }
				discard_card(hero_id, cid);
				++discarded;
			}
			if (discarded > 0) { draw_n_cards(hero_id, discarded); }
		}
		break;
	}

	// Advance hero timeline
	_store.common(hero_id).timeline_position += cost;
	_mission_events.advance_time(cost);
	emit(EVT_MISSION_TIME, hero_id,
	     std::to_string(_store.common(hero_id).timeline_position));

	// Formation check in the hero's current location
	check_formation(_store.common(hero_id).area_id);

	emit(EVT_PG_SIMPLE_ACTION, hero_id,
		 std::to_string(static_cast<int>(action_type)));

	// Defer end_hero_turn if formation dialogs are queued (main.cpp handles them).
	if (!_formation_queue.empty())
	{
		_formation_ends_turn = true;
		_formation_turn_hero = hero_id;
		return {};
	}

	end_hero_turn(hero_id);
	return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// PG turn — play_card
// ─────────────────────────────────────────────────────────────────────────────

ActionResult EldhomEngine::play_card(
	const HeroId&               hero_id,
	const CardId&               card_id,
	const LocationId&           destination,
	const std::vector<CardId>&  discard_ids,
	const gmActor::ActorId&     target_id)
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

	// Positional requirement (e.g. Fendente Pesante, Spinta di Corpo): the
	// caster must be in FRONTLINE before any effect is applied.
	if (card.requires_frontline &&
	    c.area_position != gmActor::AreaPosition::FRONTLINE)
	{
		return { ActionResultCode::ERR_POSITION_REQUIRED,
		         hero_id + " must be in FRONTLINE to play " + card_id };
	}

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

	// Explicit player-chosen target (e.g. the enemy clicked in the GUI,
	// mirroring declare_attack() for Attacco Semplice): validate it BEFORE
	// applying any effect, so the action stays atomic (either everything
	// below succeeds, or nothing is mutated — no charged cost / discarded
	// card left behind on a rejected target). Pre-scan the card's own
	// effects to find the DAMAGE range and, if a MOVE effect precedes it
	// (e.g. Passo e Lama), the location the caster will be in once the
	// DAMAGE effect resolves — validation must use THAT location, not the
	// caster's current one.
	if (!target_id.empty())
	{
		LocationId effective_loc = c.area_id;
		int        pre_range     = 0;
		bool       pre_has_damage = false;
		for (const EldhomEffect& eff : card.effects)
		{
			if (eff.effect_type == "MOVE")
			{
				const LocationId move_to = (!destination.empty() &&
				                            (eff.value.empty() ||
				                             eff.target == "ADJACENT_LOCATION" ||
				                             eff.target == "PLAYER_CHOICE"))
				                           ? destination
				                           : (!eff.value.empty() ? eff.value : eff.target);
				if (!move_to.empty()) { effective_loc = move_to; }
				continue;
			}
			if (eff.effect_type == "DAMAGE" || eff.effect_type == "DEAL_DAMAGE")
			{
				pre_has_damage = true;
				pre_range      = eff.range;
				break;
			}
		}
		if (pre_has_damage &&
		    (!_store.has_actor(target_id) ||
		     !_rule_adapter.is_valid_target_in_range(
		         _store, effective_loc, target_id, enemy_faction, pre_range)))
		{
			return { ActionResultCode::ERR_NO_VALID_TARGET,
			         "Invalid target: " + target_id };
		}
	}

	// ── Apply effects ──────────────────────────────────────────────────────────
	// DAMAGE effects are deferred: they park a PendingAttack so the defender
	// can react interactively (instant window + defense window).  All other
	// effect types are applied immediately before the card is committed.

	bool        has_damage    = false;
	int         damage_amount = 0;
	int         damage_range  = 0;  ///< Attack range declared by the DAMAGE effect (0 = mischia)
	bool        has_disrupt   = false;
	int         extra_move_cost = 0;  ///< +1 per still-closed zone door crossed by MOVE effects
	// Colpo Secco "parte A": pesca/scarta solo se attaccante E bersaglio sono
	// entrambi in FRONTLINE. Il bersaglio non è ancora noto in questo loop
	// (risolto più sotto insieme al DAMAGE deferred), quindi la valutazione è
	// rimandata a dopo find_nearest_target.
	bool        has_conditional_discard_draw   = false;
	int         conditional_discard_amount    = 0;

	for (const EldhomEffect& eff : card.effects)
	{
		if (eff.effect_type == "DAMAGE" || eff.effect_type == "DEAL_DAMAGE")
		{
			has_damage    = true;
			damage_amount = eff.amount;
			damage_range  = eff.range;
			continue; // deferred — applied via reaction chain
		}
		if (eff.effect_type == "DISRUPT_ENEMY_FORMATION")
		{
			has_disrupt = true;
			continue; // marker only — queued after resolve_reaction
		}
		if (eff.effect_type == "DRAW_CARD")
		{
			draw_n_cards(hero_id, eff.amount);
			continue; // handled by engine directly
		}
		if (eff.effect_type == "DISCARD_THEN_DRAW")
		{
			if (eff.condition == "IF_BOTH_FRONTLINE")
			{
				has_conditional_discard_draw = true;
				conditional_discard_amount   = (eff.amount > 0) ? eff.amount : 1;
				continue; // evaluated after the DAMAGE target is resolved below
			}
			// Unconditional case (e.g. Riprendere Fiato): scarta fino a
			// `eff.amount` carte (raddoppiato se in RETROGUARDIA), poi ripesca
			// altrettante.
			const int base_amount = (eff.amount > 0) ? eff.amount : 1;
			const int max_discard = (c.area_position == gmActor::AreaPosition::BACKLINE)
			                        ? base_amount * 2 : base_amount;
			int discarded = 0;
			for (const CardId& cid : discard_ids)
			{
				if (discarded >= max_discard) { break; }
				discard_card(hero_id, cid);
				++discarded;
			}
			if (discarded > 0) { draw_n_cards(hero_id, discarded); }
			continue;
		}
		if (eff.effect_type == "MOVE")
		{
			// Use player-provided destination when the effect requires player choice.
			const LocationId move_to = (!destination.empty() &&
			                            (eff.value.empty() ||
			                             eff.target == "ADJACENT_LOCATION" ||
			                             eff.target == "PLAYER_CHOICE"))
			                           ? destination
			                           : (!eff.value.empty() ? eff.value : eff.target);
			// Cards with amount > 1 allow multi-step BFS movement.
			EffectResult mres = (eff.amount > 1)
				? _rule_adapter.apply_card_move(hero_id, move_to, eff.amount, _store,
				                                 eff.avoid_enemy_locations, enemy_faction)
				: _rule_adapter.apply_simple_move(hero_id, move_to, _store);
			if (mres.resolved)
			{
				extra_move_cost += mres.extra_timeline_cost;
				for (const std::pair<LocationId, LocationId>& door : mres.opened_doors)
				{
					emit(EVT_ZONE_DOOR_OPENED, hero_id, zone_door_payload(door));
				}
				emit(EVT_PG_MOVED, hero_id, move_to);
				check_pg_at_location(hero_id, move_to);
			}
			continue;
		}
		if (eff.effect_type == "INTERACT")
		{
			trigger_special_object(hero_id);
			continue;
		}
		EffectResult res =
			_rule_adapter.apply_effect(eff, hero_id, _store, enemy_faction);
		if (res.target_ko && !res.target_id.empty())
		{
			emit(EVT_MONSTER_DEFEATED, res.target_id, {});
			handle_monster_instance_death(res.target_id);
		}
	}

	// Save original state for is_turn_ending check BEFORE advancing
	gmAlea::CardType      original_card_type = card.card_type;
	gmAlea::SequenceState seq_before         = seq; // copy (value semantics)

	// Advance sequence state
	_seq_states[hero_id] = _sequence_adapter.advance(original_card_type, seq_before);

	// Advance timeline
	const int total_card_cost = card.timeline_cost + extra_move_cost;
	_store.common(hero_id).timeline_position += total_card_cost;
	_mission_events.advance_time(total_card_cost);
	emit(EVT_MISSION_TIME, hero_id,
	     std::to_string(_store.common(hero_id).timeline_position));

	// Move played card from hand to the current-turn played zone.
	// Played cards are only transferred to the discard pile at end of turn
	// (end_hero_turn), preventing them from being reshuffled and redrawn
	// during the same turn by DRAW_CARD effects.
	if (_hand_states.count(hero_id))
	{
		HeroHandState& hs = _hand_states.at(hero_id);
		auto pos = std::find(hs.hand.begin(), hs.hand.end(), card_id);
		if (pos != hs.hand.end())
		{
			hs.played.push_back(*pos);
			hs.hand.erase(pos);
		}
	}

	emit(EVT_PG_PLAYED_CARD, hero_id, card_id);

	// Formation check
	check_formation(_store.common(hero_id).area_id);

	// ── DAMAGE deferred path: park pending attack ─────────────────────────────
	if (has_damage)
	{
		const LocationId& loc = _store.common(hero_id).area_id;
		// Prefer the player-chosen target (already validated above, before
		// any effect was applied); fall back to automatic nearest-target
		// selection when the caller didn't specify one.
		gmActor::ActorId target = !target_id.empty()
			? target_id
			: _rule_adapter.find_nearest_target(_store, loc, enemy_faction, damage_range);

		if (!target.empty())
		{
			// Colpo Secco "parte A": applica pesca/scarta solo se l'attaccante
			// e il bersaglio sono entrambi in FRONTLINE al momento dell'attacco.
			if (has_conditional_discard_draw &&
			    c.area_position == gmActor::AreaPosition::FRONTLINE &&
			    _store.common(target).area_position == gmActor::AreaPosition::FRONTLINE)
			{
				int discarded = 0;
				for (const CardId& cid : discard_ids)
				{
					if (discarded >= conditional_discard_amount) { break; }
					discard_card(hero_id, cid);
					++discarded;
				}
				if (discarded > 0) { draw_n_cards(hero_id, discarded); }
			}

			// Timeline was already charged above; set attack_cost = 0 so that
			// resolve_reaction does not double-charge.
			_pending.active           = true;
			_pending.attacker_id      = hero_id;
			_pending.defender_id      = target;
			_pending.base_damage      = damage_amount;
			_pending.attack_cost      = 0;
			_pending.source           = card_id;
			_pending.has_disrupt      = has_disrupt;
			_pending.instant_trigger  = EVT_MONSTER_DAMAGED;
			_pending.awaiting_instants =
				!eligible_instants(_pending.instant_trigger).empty();

			emit(EVT_ATTACK_DECLARED, hero_id, target);
			// Do NOT call end_hero_turn here: resolve_reaction will call it
			// once the reaction chain (instants → defense) is complete.
			return {};
		}
		// No target available: fall through to normal turn-ending logic.
	}

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
// Interactive attack / reaction window (§5.5)
// ─────────────────────────────────────────────────────────────────────────────

ActionResult EldhomEngine::declare_attack(
	const HeroId&           hero_id,
	const gmActor::ActorId& target_id)
{
	if (_mission_events.is_over()) { return { ActionResultCode::OK, "Mission over" }; }

	if (_pending.active)
	{
		return { ActionResultCode::ERR_ATTACK_PENDING,
		         "A reaction window is already open" };
	}

	if (next_actor() != hero_id)
	{
		return { ActionResultCode::ERR_NOT_YOUR_TURN, "Not " + hero_id + "'s turn" };
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

	if (target_id.empty() || !_store.has_actor(target_id))
	{
		return { ActionResultCode::ERR_NO_VALID_TARGET,
		         "Unknown target: " + target_id };
	}

	// Validate the target is reachable in the hero's location, respecting the
	// §15 Proiezione frontline rule via the TargetingFilter.
	const std::string& faction = enemy_faction_for_hero(hero_id);
	if (faction.empty())
	{
		return { ActionResultCode::ERR_NO_VALID_TARGET,
		         hero_id + ": no enemy in location" };
	}

	const std::vector<gmActor::ActorId> targets =
		_targeting.valid_targets(_store, c.area_id, faction);
	if (std::find(targets.begin(), targets.end(), target_id) == targets.end())
	{
		return { ActionResultCode::ERR_NO_VALID_TARGET,
		         target_id + " is not a valid target (out of reach or covered)" };
	}

	// Park the attack: no effect applied, turn does not advance yet.
	_pending.active      = true;
	_pending.attacker_id = hero_id;
	_pending.defender_id = target_id;
	_pending.base_damage = 1; // Attacco Semplice = 1❌
	_pending.attack_cost = COST_SIMPLE_ATTACK;
	_pending.source      = "simple";

	// An attack on a monster may be answered by INSTANT cards reacting to
	// "eldhom.monster.damaged".  When eligible instants exist the engine opens
	// the instant window first (priority over the defender's reaction).
	_pending.instant_trigger   = EVT_MONSTER_DAMAGED;
	_pending.awaiting_instants = !eligible_instants(_pending.instant_trigger).empty();

	emit(EVT_ATTACK_DECLARED, hero_id, target_id);
	return {};
}

std::vector<InstantOption> EldhomEngine::eligible_instants(
	const std::string& trigger) const
{
	std::vector<InstantOption> out;
	if (trigger.empty()) { return out; }

	for (const auto& kv : _hand_states)
	{
		const HeroId&        holder = kv.first;
		const HeroHandState& hs     = kv.second;
		for (const CardId& cid : hs.hand)
		{
			std::unordered_map<CardId, EldhomCard>::const_iterator it =
				_card_catalog.find(cid);
			if (it == _card_catalog.end()) { continue; }
			const EldhomCard& card = it->second;
			if (card.card_type != gmAlea::CardType::INSTANT) { continue; }
			if (card.reaction_trigger != trigger) { continue; }

			InstantOption opt;
			opt.actor_id  = holder;
			opt.card_id   = cid;
			opt.card_name = card.name;
			opt.trigger   = trigger;
			out.push_back(opt);
		}
	}
	return out;
}

bool EldhomEngine::has_pending_instants() const
{
	return _pending.active && _pending.awaiting_instants;
}

ActionResult EldhomEngine::play_instants(
	const std::vector<std::pair<HeroId, CardId>>& selected)
{
	if (!_pending.active || !_pending.awaiting_instants)
	{
		return { ActionResultCode::ERR_NO_PENDING_INSTANTS, "No instant window open" };
	}

	const std::vector<InstantOption> eligible =
		eligible_instants(_pending.instant_trigger);

	// Validate every selection is eligible before applying anything.
	for (const std::pair<HeroId, CardId>& sel : selected)
	{
		bool ok = false;
		for (const InstantOption& o : eligible)
		{
			if (o.actor_id == sel.first && o.card_id == sel.second)
			{
				ok = true;
				break;
			}
		}
		if (!ok)
		{
			return { ActionResultCode::ERR_INSTANT_NOT_ELIGIBLE,
			         "Instant not eligible: " + sel.second };
		}
	}

	const std::string& enemy_faction = enemy_faction_for_hero(_pending.attacker_id);

	for (const std::pair<HeroId, CardId>& sel : selected)
	{
		const HeroId& holder = sel.first;
		const CardId& cid    = sel.second;

		std::unordered_map<CardId, EldhomCard>::const_iterator it =
			_card_catalog.find(cid);
		if (it == _card_catalog.end()) { continue; }
		const EldhomCard& card = it->second;

		for (const EldhomEffect& eff : card.effects)
		{
			if (eff.effect_type == "REDUCE_DAMAGE")
			{
				_pending.base_damage = std::max(0, _pending.base_damage - eff.amount);
				continue;
			}
			EffectResult res =
				_rule_adapter.apply_effect(eff, holder, _store, enemy_faction);
			if (res.target_ko && !res.target_id.empty())
			{
				emit(EVT_MONSTER_DEFEATED, res.target_id, {});
				handle_monster_instance_death(res.target_id);
			}
		}

		// Advance the playing actor's timeline by the instant's cost.
		if (_store.has_actor(holder))
		{
			_store.common(holder).timeline_position += card.timeline_cost;
		}
		_mission_events.advance_time(card.timeline_cost);
		if (_store.has_actor(holder))
		{
			emit(EVT_MISSION_TIME, holder,
			     std::to_string(_store.common(holder).timeline_position));
		}

		// Move the instant from hand to discard.
		std::unordered_map<HeroId, HeroHandState>::iterator hsit =
			_hand_states.find(holder);
		if (hsit != _hand_states.end())
		{
			std::vector<CardId>& h = hsit->second.hand;
			std::vector<CardId>::iterator pos = std::find(h.begin(), h.end(), cid);
			if (pos != h.end())
			{
				hsit->second.discard.push_back(*pos);
				h.erase(pos);
			}
		}

		emit(EVT_PG_PLAYED_CARD, holder, cid);
	}

	emit(EVT_MISSION_TIME, {}, std::to_string(_mission_events.mission_time()));

	// Instants resolved: close the instant stage.  When a pending attack is
	// open the defender still reacts next (defense window).
	_pending.awaiting_instants = false;
	return {};
}

// ── Reactive instant window (Assestarsi — enemy approach) ─────────────────────

bool EldhomEngine::has_pending_reactive_window() const
{
	return _reactive_window.active;
}

const PendingReactiveWindow& EldhomEngine::pending_reactive_window() const
{
	return _reactive_window;
}

void EldhomEngine::maybe_open_enemy_approach_window(const LocationId& enemy_loc)
{
	// Do not overwrite an already-open window (e.g. two monsters moving in
	// the same group turn); the first trigger wins for this simplified pass.
	if (_reactive_window.active) { return; }

	std::vector<LocationId> check_locs{enemy_loc};
	auto adj_it = _adjacency.find(enemy_loc);
	if (adj_it != _adjacency.end())
	{
		check_locs.insert(check_locs.end(), adj_it->second.begin(), adj_it->second.end());
	}

	bool pg_nearby = false;
	for (const LocationId& loc : check_locs)
	{
		for (const auto& kv : _store.heroes())
		{
			if (kv.second.common.area_id == loc &&
			    kv.second.common.life_state == gmActor::ActorLifeState::ACTIVE)
			{
				pg_nearby = true;
				break;
			}
		}
		if (pg_nearby) { break; }
	}
	if (!pg_nearby) { return; }

	if (eligible_instants(EVT_ENEMY_APPROACH).empty()) { return; }

	_reactive_window.active      = true;
	_reactive_window.trigger     = EVT_ENEMY_APPROACH;
	_reactive_window.location_id = enemy_loc;
	emit(EVT_INSTANT_WINDOW_OPEN, {}, enemy_loc);
}

ActionResult EldhomEngine::play_reactive_instants(
	const std::vector<std::pair<HeroId, CardId>>& selected)
{
	if (!_reactive_window.active)
	{
		return { ActionResultCode::ERR_NO_PENDING_INSTANTS, "No reactive window open" };
	}

	const std::vector<InstantOption> eligible =
		eligible_instants(_reactive_window.trigger);

	for (const std::pair<HeroId, CardId>& sel : selected)
	{
		bool ok = false;
		for (const InstantOption& o : eligible)
		{
			if (o.actor_id == sel.first && o.card_id == sel.second) { ok = true; break; }
		}
		if (!ok)
		{
			return { ActionResultCode::ERR_INSTANT_NOT_ELIGIBLE,
			         "Instant not eligible: " + sel.second };
		}
	}

	for (const std::pair<HeroId, CardId>& sel : selected)
	{
		const HeroId& holder = sel.first;
		const CardId& cid    = sel.second;

		std::unordered_map<CardId, EldhomCard>::const_iterator it =
			_card_catalog.find(cid);
		if (it == _card_catalog.end()) { continue; }
		const EldhomCard& card = it->second;

		// Reactive-window cards today only need FORMATION_PUSH/WAIT-style
		// effects (e.g. Assestarsi); no target faction is needed since there
		// is no attack to resolve.
		for (const EldhomEffect& eff : card.effects)
		{
			_rule_adapter.apply_effect(eff, holder, _store, std::string{});
		}

		if (_store.has_actor(holder))
		{
			_store.common(holder).timeline_position += card.timeline_cost;
			check_formation(_store.common(holder).area_id);
		}
		_mission_events.advance_time(card.timeline_cost);
		if (_store.has_actor(holder))
		{
			emit(EVT_MISSION_TIME, holder,
			     std::to_string(_store.common(holder).timeline_position));
		}

		discard_card(holder, cid);
		emit(EVT_PG_PLAYED_CARD, holder, cid);
	}

	_reactive_window = PendingReactiveWindow{};
	emit(EVT_INSTANT_WINDOW_CLOSED, {}, {});
	return {};
}

std::vector<DefenseReaction> EldhomEngine::allowed_reactions() const
{
	std::vector<DefenseReaction> out;
	if (!_pending.active) { return out; }

	out.push_back(DefenseReaction::TAKE);
	out.push_back(DefenseReaction::BLOCK);

	// DODGE (Schiva) is offered only when the defender can retreat to BACKLINE.
	if (_store.has_actor(_pending.defender_id))
	{
		const gmActor::ActorStateCommon& d = _store.common(_pending.defender_id);
		if (d.area_position == gmActor::AreaPosition::FRONTLINE)
		{
			out.push_back(DefenseReaction::DODGE);
		}
	}
	return out;
}

bool EldhomEngine::has_pending_attack() const
{
	return _pending.active;
}

const PendingAttack& EldhomEngine::pending_attack() const
{
	return _pending;
}

ActionResult EldhomEngine::resolve_reaction(
	const gmActor::ActorId& defender_id,
	DefenseReaction         reaction,
	ReactionResolution*     out)
{
	if (!_pending.active)
	{
		return { ActionResultCode::ERR_NO_PENDING_ATTACK, "No attack pending" };
	}

	if (defender_id != _pending.defender_id)
	{
		return { ActionResultCode::ERR_NOT_DEFENDER,
		         defender_id + " is not the pending defender" };
	}

	// Validate the chosen reaction is allowed for this defender.
	const std::vector<DefenseReaction> allowed = allowed_reactions();
	if (std::find(allowed.begin(), allowed.end(), reaction) == allowed.end())
	{
		return { ActionResultCode::ERR_REACTION_NOT_ALLOWED,
		         "Reaction " + to_string(reaction) + " not allowed" };
	}

	const HeroId           attacker_id  = _pending.attacker_id;
	const gmActor::ActorId target_id    = _pending.defender_id;
	const int              base_damage  = _pending.base_damage;
	const int              cost         = _pending.attack_cost;
	const bool             had_disrupt  = _pending.has_disrupt;

	// Compute the final damage and apply any positional side-effect.
	int final_damage = base_damage;
	switch (reaction)
	{
	case DefenseReaction::TAKE:
		final_damage = base_damage;
		break;
	case DefenseReaction::BLOCK:
		final_damage = std::max(0, base_damage - REACTION_BLOCK_REDUCTION);
		break;
	case DefenseReaction::DODGE:
		final_damage = 0;
		// Schiva: the defender retreats to the BACKLINE (may cause Scompaginamento).
		_store.common(target_id).area_position = gmActor::AreaPosition::BACKLINE;
		emit(EVT_FORMATION_CHANGED, target_id, "BACKLINE");
		break;
	}

	EffectResult eff;
	if (final_damage > 0)
	{
		eff = _rule_adapter.deal_damage(target_id, final_damage, _store);
	}
	else
	{
		eff.resolved  = true;
		eff.target_id = target_id;
	}

	// Read the defender's HP after the attack (0 if the actor was removed).
	int defender_hp_after = 0;
	if (_store.has_actor(target_id))
	{
		defender_hp_after = _store.common(target_id).current_hp;
	}

	// Close the reaction window before applying turn-advancement side effects.
	_pending = PendingAttack{};

	{
		const int log_dmg = (eff.damage_dealt > 0) ? eff.damage_dealt : final_damage;
		std::string atk_payload =
			std::string("{\"target\":\"") + target_id
			+ "\",\"damage\":" + std::to_string(log_dmg)
			+ ",\"type\":\"MELEE\",\"range\":0}";
		emit(EVT_PG_ATTACKED, attacker_id, atk_payload);
	}
	if (eff.target_ko)
	{
		emit(EVT_MONSTER_DEFEATED, target_id, {});
		handle_monster_instance_death(target_id);
	}

	// Charge the attacker's timeline and advance the mission clock.
	_store.common(attacker_id).timeline_position += cost;
	_mission_events.advance_time(cost);
	emit(EVT_MISSION_TIME, attacker_id,
	     std::to_string(_store.common(attacker_id).timeline_position));

	// Formation check in the attacker's location (defender may have retreated).
	// For DISRUPT: replace any enemy-faction dialog from check_formation with the
	// unconditional disrupt dialog (player reorganises enemy regardless of validity).
	if (had_disrupt)
	{
		const std::string enemy_fac = enemy_faction_for_hero(attacker_id);
		const LocationId& atk_loc   = _store.common(attacker_id).area_id;

		// Run check_formation for PG factions only (enemy handled via disrupt)
		check_formation(atk_loc);

		// Remove any auto-queued entry for the enemy faction (if formation was
		// already invalid from DODGE), then add the explicit disrupt entry.
		_formation_queue.erase(
			std::remove_if(_formation_queue.begin(), _formation_queue.end(),
			               [&](const PendingFormation& pf) {
			                   return pf.faction_id == enemy_fac &&
			                          pf.location_id == atk_loc;
			               }),
			_formation_queue.end());
		queue_enemy_disrupt(attacker_id);
	}
	else
	{
		check_formation(_store.common(attacker_id).area_id);
	}

	emit(EVT_PG_SIMPLE_ACTION, attacker_id,
		 std::to_string(static_cast<int>(SimpleActionType::ATTACK)));

	// Defer end_hero_turn if formation dialogs are queued (main.cpp handles them).
	if (!_formation_queue.empty())
	{
		_formation_ends_turn = true;
		_formation_turn_hero = attacker_id;
	}
	else
	{
		end_hero_turn(attacker_id);
	}

	if (out)
	{
		out->ok                = true;
		out->attacker_id       = attacker_id;
		out->defender_id       = target_id;
		out->base_damage       = base_damage;
		out->final_damage      = final_damage;
		out->reaction          = reaction;
		out->defender_hp_after = defender_hp_after;
		out->defender_ko       = eff.target_ko;
	}

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

			// Log monster movement.
			const bool is_move_step =
				(step.effect_type == "MOVE_TOWARD_PG" ||
				 step.effect_type == "MOVE_TOWARD_NEAREST_PG");
			if (is_move_step && res.resolved && !res.target_id.empty())
			{
				// res.target_id == member_id when the monster actually moved
				// (empty when already in contact). New location is already stored.
				const LocationId& new_loc = st.common(member_id).area_id;
				engine->emit(EVT_MONSTER_MOVED, member_id, new_loc);
				// Assestarsi-style reactive window: give nearby PGs a chance
				// to react before the group's turn is considered finished.
				engine->maybe_open_enemy_approach_window(new_loc);
			}

			// Log monster attacks on PG heroes.
			if (res.resolved && !res.target_id.empty() && res.damage_dealt > 0)
			{
				// Determine attack type from BehaviorStep:
				// Convention: step.value=="RANGED:N" means ranged attack, range=N.
				std::string atk_type = "MELEE";
				int         atk_range = 0;
				if (step.value.find("RANGED") != std::string::npos)
				{
					atk_type = "RANGED";
					std::size_t colon = step.value.find(':');
					if (colon != std::string::npos)
					{
						try { atk_range = std::stoi(step.value.substr(colon + 1)); }
						catch (...) {}
					}
				}
				// Build compact JSON payload (actor IDs are plain ASCII).
				std::string atk_payload =
					std::string("{\"target\":\"") + res.target_id
					+ "\",\"damage\":" + std::to_string(res.damage_dealt)
					+ ",\"type\":\"" + atk_type
					+ "\",\"range\":" + std::to_string(atk_range) + "}";
				engine->emit(EVT_PG_ATTACKED, member_id, atk_payload);
			}

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
// Formation dialog API
// ─────────────────────────────────────────────────────────────────────────────

bool EldhomEngine::has_pending_formation() const
{
	return !_formation_queue.empty();
}

const PendingFormation& EldhomEngine::current_formation_dialog() const
{
	return _formation_queue.front();
}

ActionResult EldhomEngine::resolve_formation(
	const std::string&                   faction_id,
	const LocationId&                    location_id,
	const std::vector<gmActor::ActorId>& backline_ids)
{
	if (_formation_queue.empty())
	{
		return { ActionResultCode::ERR_NO_PENDING_FORMATION,
		         "No formation dialog is open" };
	}

	const PendingFormation& pf = _formation_queue.front();

	int total       = static_cast<int>(pf.actors.size());
	int back_count  = static_cast<int>(backline_ids.size());
	int front_count = total - back_count;

	if (back_count > front_count)
	{
		return { ActionResultCode::ERR_INVALID_FORMATION_CHOICE,
		         "Retroguardia non puo' essere maggiore di Prima Linea" };
	}

	// Apply chosen formation to heroes in the location
	for (const auto& kv : _store.heroes())
	{
		gmActor::HeroState& h = _store.hero(kv.first);
		if (h.common.faction_id != faction_id)  { continue; }
		if (h.common.area_id    != location_id) { continue; }
		if (h.common.removed)                   { continue; }
		bool in_back = std::find(backline_ids.begin(), backline_ids.end(),
		                         kv.first) != backline_ids.end();
		h.common.area_position = in_back
			? gmActor::AreaPosition::BACKLINE
			: gmActor::AreaPosition::FRONTLINE;
	}

	// Apply chosen formation to monster instances in the location
	for (const auto& kv : _store.monster_instances())
	{
		gmActor::MonsterInstanceState& m = _store.monster_instance(kv.first);
		if (m.common.faction_id != faction_id)  { continue; }
		if (m.common.area_id    != location_id) { continue; }
		if (m.common.removed)                   { continue; }
		bool in_back = std::find(backline_ids.begin(), backline_ids.end(),
		                         kv.first) != backline_ids.end();
		m.common.area_position = in_back
			? gmActor::AreaPosition::BACKLINE
			: gmActor::AreaPosition::FRONTLINE;
	}

	emit(EVT_FORMATION_CHANGED, {},
	     faction_id + "@" + location_id + ":player_resolved");

	_formation_queue.pop_front();

	if (!_formation_queue.empty())
	{
		// More dialogs pending; main.cpp will emit the next one.
		return {};
	}

	// Queue exhausted: end the deferred hero turn if applicable.
	if (_formation_ends_turn && !_formation_turn_hero.empty())
	{
		end_hero_turn(_formation_turn_hero);
		_formation_ends_turn = false;
		_formation_turn_hero = {};
	}

	return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void EldhomEngine::check_formation(const LocationId& location_id)
{
	// Check all known factions in this location.  For invalid formations, queue
	// an interactive dialog instead of auto-fixing (player decides who moves).
	std::vector<std::string> all_factions;
	for (const std::string& f : _hero_factions)    { all_factions.push_back(f); }
	for (const std::string& f : _monster_factions) { all_factions.push_back(f); }

	for (const std::string& faction : all_factions)
	{
		FormationCheckResult res =
			_formation_adapter.check(_store, location_id, faction);

		if (!res.scompaginamento && res.valid) { continue; }

		// Build the actor list for the formation dialog.
		PendingFormation pf;
		pf.location_id = location_id;
		pf.faction_id  = faction;
		pf.source      = res.scompaginamento ? "scompaginamento" : "overflow";

		for (const auto& kv : _store.heroes())
		{
			const gmActor::HeroState& h = kv.second;
			if (h.common.faction_id != faction)     { continue; }
			if (h.common.area_id    != location_id) { continue; }
			if (h.common.removed)                   { continue; }
			ActorFormationEntry e;
			e.actor_id     = kv.first;
			e.display_name = kv.first;
			e.in_backline  = (h.common.area_position == gmActor::AreaPosition::BACKLINE);
			pf.actors.push_back(e);
		}
		for (const auto& kv : _store.monster_instances())
		{
			const gmActor::MonsterInstanceState& m = kv.second;
			if (m.common.faction_id != faction)     { continue; }
			if (m.common.area_id    != location_id) { continue; }
			if (m.common.removed)                   { continue; }
			ActorFormationEntry e;
			e.actor_id     = kv.first;
			e.display_name = kv.first;
			e.in_backline  = (m.common.area_position == gmActor::AreaPosition::BACKLINE);
			pf.actors.push_back(e);
		}

		if (!pf.actors.empty())
		{
			_formation_queue.push_back(std::move(pf));
		}
	}
}

void EldhomEngine::queue_enemy_disrupt(const HeroId& attacker_id)
{
	if (!_store.has_actor(attacker_id)) { return; }

	const LocationId& loc        = _store.common(attacker_id).area_id;
	const std::string enemy_fac  = enemy_faction_for_hero(attacker_id);
	if (enemy_fac.empty()) { return; }

	PendingFormation pf;
	pf.location_id = loc;
	pf.faction_id  = enemy_fac;
	pf.source      = "disrupt";

	for (const auto& kv : _store.monster_instances())
	{
		const gmActor::MonsterInstanceState& m = kv.second;
		if (m.common.faction_id != enemy_fac) { continue; }
		if (m.common.area_id    != loc)       { continue; }
		if (m.common.removed)                 { continue; }
		ActorFormationEntry e;
		e.actor_id     = kv.first;
		e.display_name = kv.first;
		e.in_backline  = (m.common.area_position == gmActor::AreaPosition::BACKLINE);
		pf.actors.push_back(e);
	}

	if (!pf.actors.empty())
	{
		_formation_queue.push_back(std::move(pf));
	}
}

void EldhomEngine::draw_n_cards(const HeroId& hero_id, int n)
{
	auto it = _hand_states.find(hero_id);
	if (it == _hand_states.end()) { return; }
	HeroHandState& hs = it->second;

	for (int i = 0; i < n; ++i)
	{
		if (hs.deck.empty() && !hs.discard.empty())
		{
			hs.deck.insert(hs.deck.end(), hs.discard.begin(), hs.discard.end());
			hs.discard.clear();
			emit(EVT_DECK_RESHUFFLED, hero_id, {});
		}
		if (hs.deck.empty()) { break; }
		hs.hand.push_back(hs.deck.back());
		hs.deck.pop_back();
	}
	emit(EVT_HAND_CHANGED, hero_id, {});
}

void EldhomEngine::discard_card(const HeroId& hero_id, const CardId& card_id)
{
	auto it = _hand_states.find(hero_id);
	if (it == _hand_states.end()) { return; }
	HeroHandState& hs = it->second;
	auto pos = std::find(hs.hand.begin(), hs.hand.end(), card_id);
	if (pos == hs.hand.end()) { return; }
	hs.hand.erase(pos);
	hs.discard.push_back(card_id);
	emit(EVT_HAND_CHANGED, hero_id, {});
}

void EldhomEngine::take_from_discard(const HeroId& hero_id)
{
	auto it = _hand_states.find(hero_id);
	if (it == _hand_states.end()) { return; }
	HeroHandState& hs = it->second;
	if (hs.discard.empty()) { return; }
	hs.hand.push_back(hs.discard.back());
	hs.discard.pop_back();
	emit(EVT_HAND_CHANGED, hero_id, {});
}

void EldhomEngine::reshuffle_discard(const HeroId& hero_id)
{
	auto it = _hand_states.find(hero_id);
	if (it == _hand_states.end()) { return; }
	HeroHandState& hs = it->second;
	if (hs.discard.empty()) { return; }
	hs.deck.insert(hs.deck.end(), hs.discard.begin(), hs.discard.end());
	hs.discard.clear();
	std::mt19937 rng(std::random_device{}());
	std::shuffle(hs.deck.begin(), hs.deck.end(), rng);
	emit(EVT_DECK_RESHUFFLED, hero_id, {});
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

	// Flush cards played this turn from the played zone to the discard pile
	// BEFORE drawing, so they are not immediately recycled by a reshuffle.
	if (_hand_states.count(hero_id))
	{
		HeroHandState& hs = _hand_states.at(hero_id);
		for (const CardId& cid : hs.played)
		{
			hs.discard.push_back(cid);
		}
		hs.played.clear();
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
	// Fallback: no enemy of any monster faction stands in the hero's CURRENT
	// location (e.g. before a MOVE effect relocates them). Cards that combine
	// MOVE + DAMAGE (Passo e Lama) or MOVE with avoid_enemy_locations (Passo
	// Cauto, Scatto Breve) still need to know which faction to target/avoid
	// along the way, so default to the mission's (first) monster faction
	// rather than an empty string.
	if (!_monster_factions.empty()) { return _monster_factions.front(); }
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

// ─────────────────────────────────────────────────────────────────────────────
// Special object interaction
// ─────────────────────────────────────────────────────────────────────────────

void EldhomEngine::trigger_special_object(const HeroId& hero_id)
{
	if (!_store.has_actor(hero_id)) { return; }
	const std::string& hero_loc = _store.common(hero_id).area_id;

	for (SpecialObject& obj : _special_objects)
	{
		if (obj.location_id != hero_loc) { continue; }
		if (_special_object_used[obj.object_id]) { continue; }

		_special_object_used[obj.object_id] = true;

		// Unlock adjacency pairs for all interaction types
		for (const std::pair<LocationId, LocationId>& pair : obj.on_interact.adjacency)
		{
			_rule_adapter.add_adjacency(pair.first, pair.second);
		}

		if (obj.type == "LEVER")
		{
			emit(EVT_PORTA_APERTA, hero_id, obj.object_id);
		}
		else if (obj.type == "PICKUP_TESORO")
		{
			_tesoro_carrier = hero_id;
			emit(EVT_TESORO_RACCOLTO, hero_id, obj.object_id);
			emit(EVT_ALLARME_TESORO, hero_id, {});
			emit(EVT_PASSAGGIO_APERTO, hero_id, obj.object_id);
		}
		break; // only one object per location is triggered per interaction
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// PG_REACHED_EXIT check
// ─────────────────────────────────────────────────────────────────────────────

void EldhomEngine::check_pg_at_location(
	const HeroId&     hero_id,
	const LocationId& new_loc)
{
	const std::string item_carried =
		(_tesoro_carrier == hero_id) ? "tesoro" : "";
	_mission_events.notify_pg_moved(hero_id, new_loc, item_carried);
}

} // namespace eldhom
