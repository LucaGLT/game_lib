/**
 * @file tests/test_eldhom_mission01.cpp
 * @brief Functional test for Le Pergamene di Eldhom — P6 (Turno PG + Turno Gruppo Mostri).
 *
 * Tests the full turn cycle:
 *   1. Thael (FRONTLINE, ingresso) plays base_colpo_secco → no enemy in location → move first
 *   2. Thael moves to corridoio (Mossa Tattica)
 *   3. Thael attacks (base_colpo_secco) → hits brigante_A1 (1 damage)
 *   4. Velyr moves to corridoio
 *   5. Briganti A activate (Assalto card) → attack Thael
 *   6. Briganti B stay in sala (no PGs nearby)
 *   7. Continue until all Briganti eliminated → VICTORY
 *   8. Check defeat by time limit (separate scenario)
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomEngine.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"
#include "GAME/Eldhom/CoreEngine/engine/CardData.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionDefinition.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionEventSystem.hpp"

#include "gmActor/behavior/BehaviorCard.hpp"
#include "gmActor/behavior/BehaviorStep.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Test helpers
// ─────────────────────────────────────────────────────────────────────────────

static int s_pass = 0;
static int s_fail = 0;

void check(bool condition, const std::string& label)
{
	if (condition)
	{
		std::cout << "  [PASS] " << label << "\n";
		++s_pass;
	}
	else
	{
		std::cout << "  [FAIL] " << label << "\n";
		++s_fail;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Card catalog builder
// ─────────────────────────────────────────────────────────────────────────────

eldhom::EldhomCard make_card(
	const eldhom::CardId&    id,
	const std::string&       name,
	gmAlea::CardType         type,
	int                      cost,
	const std::vector<eldhom::EldhomEffect>& effects)
{
	eldhom::EldhomCard c;
	c.card_id       = id;
	c.name          = name;
	c.card_type     = type;
	c.timeline_cost = cost;
	c.effects       = effects;
	return c;
}

std::unordered_map<eldhom::CardId, eldhom::EldhomCard> build_card_catalog()
{
	std::unordered_map<eldhom::CardId, eldhom::EldhomCard> cat;

	// base_colpo_secco: SINGLE, 2⌛, DAMAGE 1 nearest enemy
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "ENEMY_FRONTLINE";
		cat["base_colpo_secco"] =
			make_card("base_colpo_secco", "Colpo Secco",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	// base_mossa_tattica: SINGLE, 1⌛, MOVE (destination passed externally)
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "MOVE";
		eff.amount      = 1;
		cat["base_mossa_tattica"] =
			make_card("base_mossa_tattica", "Mossa Tattica",
			          gmAlea::CardType::SINGLE, 1, { eff });
	}

	// base_attacco_inizio_seq: SEQ_START, 2⌛, DAMAGE 1
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		cat["base_attacco_inizio_seq"] =
			make_card("base_attacco_inizio_seq", "Attacco Inizio",
			          gmAlea::CardType::SEQ_START, 2, { eff });
	}

	// base_attacco_chiudi_seq: SEQ_END, 2⌛, DAMAGE 2
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 2;
		cat["base_attacco_chiudi_seq"] =
			make_card("base_attacco_chiudi_seq", "Attacco Chiudi",
			          gmAlea::CardType::SEQ_END, 2, { eff });
	}

	// velyr_cura_rapida: SINGLE, 2⌛, HEAL 2 self
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "HEAL";
		eff.amount      = 2;
		eff.target      = "SELF";
		cat["velyr_cura_rapida"] =
			make_card("velyr_cura_rapida", "Cura Rapida",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	return cat;
}

// ─────────────────────────────────────────────────────────────────────────────
// Behavior card catalog builder
// ─────────────────────────────────────────────────────────────────────────────

std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> build_behavior_catalog()
{
	std::unordered_map<gmActor::CardId, gmActor::BehaviorCard> cat;

	// brigante_assalto: MOVE_TOWARD_PG (optional,1⌛) → DEAL_DAMAGE (mandatory,2⌛)
	{
		gmActor::BehaviorCard card;
		card.card_id = "brigante_assalto";

		gmActor::BehaviorStep s1;
		s1.effect_type    = "MOVE_TOWARD_PG";
		s1.amount         = 1;
		s1.timeline_cost  = 1;
		s1.optional       = true;

		gmActor::BehaviorStep s2;
		s2.effect_type    = "DEAL_DAMAGE";
		s2.amount         = 1;
		s2.timeline_cost  = 2;
		s2.optional       = false;

		card.steps.push_back(s1);
		card.steps.push_back(s2);

		// fallback: just move
		gmActor::BehaviorStep fb;
		fb.effect_type   = "MOVE_TOWARD_PG";
		fb.amount        = 1;
		fb.timeline_cost = 1;
		fb.optional      = false;
		card.fallback_steps.push_back(fb);

		cat["brigante_assalto"] = card;
	}

	// brigante_avanzata_rapida: MOVE_TOWARD_PG (mandatory, 1⌛)
	{
		gmActor::BehaviorCard card;
		card.card_id = "brigante_avanzata_rapida";

		gmActor::BehaviorStep s;
		s.effect_type   = "MOVE_TOWARD_PG";
		s.amount        = 1;
		s.timeline_cost = 1;
		s.optional      = false;

		card.steps.push_back(s);

		// fallback: WAIT
		gmActor::BehaviorStep fb;
		fb.effect_type   = "WAIT";
		fb.timeline_cost = 2;
		fb.optional      = false;
		card.fallback_steps.push_back(fb);

		cat["brigante_avanzata_rapida"] = card;
	}

	return cat;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mission definition builder (mission_01 in code)
// ─────────────────────────────────────────────────────────────────────────────

eldhom::MissionDefinition build_mission_01()
{
	eldhom::MissionDefinition def;
	def.mission_id = "missione_01";
	def.title      = "L'Ombra sul Corridoio";

	// Locations
	eldhom::LocationNode ingresso;
	ingresso.id       = "ingresso";
	ingresso.adjacent = { "corridoio" };

	eldhom::LocationNode corridoio;
	corridoio.id       = "corridoio";
	corridoio.adjacent = { "ingresso", "sala" };

	eldhom::LocationNode sala;
	sala.id       = "sala";
	sala.adjacent = { "corridoio" };

	def.locations = { ingresso, corridoio, sala };

	// PGs
	eldhom::PgEntry thael;
	thael.hero_id        = "thael";
	thael.display_name   = "Thael";
	thael.class_name     = "Guerriero";
	thael.faction_id     = "HEROES";
	thael.start_location = "ingresso";
	thael.start_position = "FRONTLINE";
	thael.max_hp         = 6;
	thael.hand_limit     = 5;
	thael.level          = 1;
	thael.start_timeline = 0;

	eldhom::PgEntry velyr;
	velyr.hero_id        = "velyr";
	velyr.display_name   = "Velyr";
	velyr.class_name     = "Supporto";
	velyr.faction_id     = "HEROES";
	velyr.start_location = "ingresso";
	velyr.start_position = "BACKLINE";
	velyr.max_hp         = 5;
	velyr.hand_limit     = 5;
	velyr.level          = 1;
	velyr.start_timeline = 0;

	def.pg_roster = { thael, velyr };

	// Monster groups
	eldhom::MonsterGroupEntry briganti_A;
	briganti_A.group_id       = "briganti_A";
	briganti_A.display_name   = "Briganti A";
	briganti_A.monster_type   = "brigante_comune";
	briganti_A.faction_id     = "BRIGANTI";
	briganti_A.start_location = "corridoio";
	briganti_A.start_timeline = 4;
	briganti_A.tie_break_rank = 3;
	briganti_A.behavior_deck  = { "brigante_assalto", "brigante_avanzata_rapida",
	                               "brigante_assalto" };

	eldhom::MonsterInstanceEntry ba1;
	ba1.instance_id = "brigante_A1";
	ba1.position    = "FRONTLINE";
	ba1.max_hp      = 3;
	ba1.damage      = 1;
	ba1.movement    = 2;

	eldhom::MonsterInstanceEntry ba2;
	ba2.instance_id = "brigante_A2";
	ba2.position    = "BACKLINE";
	ba2.max_hp      = 1;   // low HP so we can kill it quickly
	ba2.damage      = 1;
	ba2.movement    = 2;

	briganti_A.instances = { ba1, ba2 };

	eldhom::MonsterGroupEntry briganti_B;
	briganti_B.group_id       = "briganti_B";
	briganti_B.display_name   = "Briganti B";
	briganti_B.monster_type   = "brigante_comune";
	briganti_B.faction_id     = "BRIGANTI";
	briganti_B.start_location = "sala";
	briganti_B.start_timeline = 4;
	briganti_B.tie_break_rank = 3;
	briganti_B.behavior_deck  = { "brigante_avanzata_rapida", "brigante_assalto" };

	eldhom::MonsterInstanceEntry bb1;
	bb1.instance_id = "brigante_B1";
	bb1.position    = "FRONTLINE";
	bb1.max_hp      = 1;   // easy to kill
	bb1.damage      = 1;
	bb1.movement    = 2;

	briganti_B.instances = { bb1 };

	def.monster_groups = { briganti_A, briganti_B };

	// Victory: ALL_MONSTERS_ELIMINATED
	eldhom::VictoryCondition vc;
	vc.type = "ALL_MONSTERS_ELIMINATED";
	def.victory_conditions = { vc };

	// Defeat: TIME_LIMIT 60
	eldhom::DefeatCondition dc;
	dc.type      = "TIME_LIMIT";
	dc.threshold = 60;
	def.defeat_conditions = { dc };

	// Defeat: ALL_PG_KO
	eldhom::DefeatCondition dc2;
	dc2.type      = "ALL_PG_KO";
	dc2.threshold = 0;
	def.defeat_conditions.push_back(dc2);

	return def;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test suites
// ─────────────────────────────────────────────────────────────────────────────

void test_initial_state()
{
	std::cout << "\n=== test_initial_state ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	std::vector<std::string> events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&events](const eldhom::EventType& t, const std::string& a, const std::string& p) {
			events.push_back(t + ":" + a + ":" + p);
		});

	check(engine.timeline_position("thael")     == 0,  "Thael timeline starts at 0");
	check(engine.timeline_position("velyr")     == 0,  "Velyr timeline starts at 0");
	check(engine.timeline_position("briganti_A") == 4, "Briganti_A timeline starts at 4");
	check(engine.timeline_position("briganti_B") == 4, "Briganti_B timeline starts at 4");

	// Lowest timeline = heroes (0 < 4), tie-break RANK_HERO=1
	check(engine.next_actor() == "thael" || engine.next_actor() == "velyr",
	      "Next actor is a hero");
	check(engine.next_actor_kind() == gmActor::ActorKind::HERO,
	      "Next actor kind is HERO");

	check(!engine.is_over(), "Mission not over at start");
	check(engine.mission_time() == 0, "Mission time starts at 0");

	const gmAlea::SequenceState& seq = engine.sequence_state("thael");
	check(!seq.active, "Thael sequence starts inactive");
}

void test_pg_turn_simple_action_move()
{
	std::cout << "\n=== test_pg_turn_simple_action_move ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Thael is next. Move to corridoio.
	eldhom::ActionResult r = engine.do_simple_action(
		"thael", eldhom::SimpleActionType::MOVE, "corridoio");

	check(r.ok(), "Thael MOVE to corridoio OK");
	// "ingresso" and "corridoio" are different zones (no shared numeric
	// suffix), so this MOVE crosses a still-closed zone-boundary door and
	// pays +1 extra timeline cost on top of COST_SIMPLE_MOVE.
	check(engine.timeline_position("thael") == eldhom::COST_SIMPLE_MOVE + 1,
	      "Thael timeline = COST_SIMPLE_MOVE + 1 after crossing a zone door");

	// Next actor should be Velyr (timeline 0 < 1)
	check(engine.next_actor() == "velyr", "Velyr is next after Thael moves");

	// Velyr can also move
	eldhom::ActionResult r2 = engine.do_simple_action(
		"velyr", eldhom::SimpleActionType::MOVE, "corridoio");
	check(r2.ok(), "Velyr MOVE to corridoio OK");
}

void test_pg_turn_not_your_turn()
{
	std::cout << "\n=== test_pg_turn_not_your_turn ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Try to play as briganti_A before their turn
	eldhom::ActionResult r = engine.do_simple_action(
		"briganti_A", eldhom::SimpleActionType::MOVE, "ingresso");
	check(r.code == eldhom::ActionResultCode::ERR_NOT_YOUR_TURN,
	      "Monster group cannot act on PG turn");
}

void test_pg_turn_play_card_single()
{
	std::cout << "\n=== test_pg_turn_play_card_single ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	std::vector<std::string> events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&events](const eldhom::EventType& t, const std::string& a, const std::string& p) {
			events.push_back(t);
		});

	// Thael moves to corridoio where enemies are
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	// Velyr stays in ingresso — use RECOVER (not a no-op MOVE to her own
	// location, which would silently fail and leave her timeline at 0,
	// making her — not Thael — the next actor).
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	// Now Thael has timeline=COST_SIMPLE_MOVE+1 (crossing the still-closed
	// ingresso<->corridoio zone door costs +1), Velyr=COST_SIMPLE_RECOVER — both 3.
	// Next is a hero (Thael or Velyr), same rank → Thael wins the tie-break.
	// Thael plays base_colpo_secco against Brigante_A1 (FRONTLINE in corridoio)
	events.clear();
	eldhom::ActionResult r = engine.play_card("thael", "base_colpo_secco");
	check(r.ok(), "Thael plays base_colpo_secco OK");
	// Start=0, +MOVE(COST_SIMPLE_MOVE+1 zone door), +card_cost(2)
	check(engine.timeline_position("thael") == eldhom::COST_SIMPLE_MOVE + 1 + 2,
	      "Thael timeline advanced by card cost");

	// Check that PG_PLAYED_CARD event was emitted
	bool played_card_event = false;
	for (const std::string& ev : events)
	{
		if (ev == eldhom::EVT_PG_PLAYED_CARD) { played_card_event = true; break; }
	}
	check(played_card_event, "EVT_PG_PLAYED_CARD was emitted");

	// DAMAGE is deferred: base_colpo_secco parked a pending attack instead of
	// applying the effect immediately. Resolve it (TAKE = no reduction) so
	// the damage is actually applied before checking brigante_A1's HP.
	check(engine.has_pending_attack(), "Colpo Secco opened a pending attack");
	engine.resolve_reaction("brigante_A1", eldhom::DefenseReaction::TAKE);

	// brigante_A1 should have taken 1 damage (max_hp=3, now hp=2)
	const gmActor::MonsterInstanceState& ba1 =
		engine.actor_store().monster_instance("brigante_A1");
	check(ba1.common.current_hp == 2, "brigante_A1 HP is now 2 after 1 damage");
}

void test_pg_turn_sequence()
{
	std::cout << "\n=== test_pg_turn_sequence ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Thael moves to corridoio
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	// Velyr stays in ingresso — RECOVER, not a no-op MOVE to her own location
	// (see test_pg_turn_play_card_single for why that silently fails).
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	// Thael plays SEQ_START — turn should NOT end
	eldhom::ActionResult r1 = engine.play_card("thael", "base_attacco_inizio_seq");
	check(r1.ok(), "SEQ_START played OK");

	const gmAlea::SequenceState& seq1 = engine.sequence_state("thael");
	check(seq1.active, "Sequence is active after SEQ_START");

	// Thael is still next (turn not over)
	check(engine.next_actor() == "thael", "Thael still next during sequence");

	// Thael plays SEQ_END — turn ends
	eldhom::ActionResult r2 = engine.play_card("thael", "base_attacco_chiudi_seq");
	check(r2.ok(), "SEQ_END played OK");

	const gmAlea::SequenceState& seq2 = engine.sequence_state("thael");
	check(!seq2.active, "Sequence closed after SEQ_END");

	// SEQ_END is turn-ending — Velyr should be next
	check(engine.next_actor() != "thael", "Thael's turn ended after SEQ_END");
}

void test_monster_group_turn()
{
	std::cout << "\n=== test_monster_group_turn ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	std::vector<std::string> events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&events](const eldhom::EventType& t, const std::string& a, const std::string&) {
			events.push_back(t + ":" + a);
		});

	// Advance PG timelines past 4 using RECOVER (cost=3) twice per hero.
	// After 2 RECOVERs each PG is at timeline=6 > 4.
	// next_actor() alternates naturally between heroes.
	for (int i = 0; i < 4; ++i)
	{
		if (engine.next_actor_kind() != gmActor::ActorKind::HERO) { break; }
		const std::string& next = engine.next_actor();
		engine.do_simple_action(next, eldhom::SimpleActionType::RECOVER);
	}

	check(engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP,
	      "Monster group is next after PGs advance past timeline=4");

	// Get Thael HP before monster attacks
	int thael_hp_before = engine.actor_store().hero("thael").common.current_hp;
	(void)thael_hp_before;

	events.clear();
	eldhom::ActionResult r = engine.resolve_next_group_turn();
	check(r.ok(), "resolve_next_group_turn OK");

	// Check group activated event
	bool group_activated = false;
	for (const std::string& ev : events)
	{
		if (ev.find(eldhom::EVT_GROUP_ACTIVATED) != std::string::npos)
		{
			group_activated = true; break;
		}
	}
	check(group_activated, "EVT_GROUP_ACTIVATED emitted");

	// Monster group timeline should have advanced
	check(engine.timeline_position("briganti_A") > 4,
	      "briganti_A timeline advanced after turn");
}

void test_victory_all_monsters_eliminated()
{
	std::cout << "\n=== test_victory_all_monsters_eliminated ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// Use low-HP monsters (1 HP each) so they die in one hit
	// Already configured that way in build_mission_01 (ba2=1HP, bb1=1HP)
	// ba1 has 3HP but colpo_secco does 1 damage — we'll use colpo_secco multiple times

	std::vector<std::string> victory_events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&victory_events](const eldhom::EventType& t,
		                  const std::string&,
		                  const std::string&) {
			if (t == eldhom::EVT_MISSION_VICTORY) { victory_events.push_back(t); }
		});

	// Thael moves to corridoio to be in range of Briganti A
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "corridoio");
	// Push ahead to make monsters go next
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "ingresso");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "ingresso");
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::MOVE, "corridoio");

	// Let briganti_A activate first (Assalto card — they are in corridoio)
	if (engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP)
	{
		engine.resolve_next_group_turn();
	}
	if (engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP)
	{
		engine.resolve_next_group_turn();
	}

	// Now kill ALL monsters with simple attacks (Thael and Velyr take turns)
	// brigante_A1: 3HP — needs 3 attacks
	// brigante_A2: 1HP — needs 1 attack (but also dies from formation targeting)
	// brigante_B1: 1HP — needs 1 attack after moving to sala

	// Run turns for up to 30 steps without a loop
	int steps = 0;
	while (!engine.is_over() && steps < 30)
	{
		++steps;
		const std::string& next = engine.next_actor();
		if (next.empty()) { break; }

		gmActor::ActorKind kind = engine.next_actor_kind();

		if (kind == gmActor::ActorKind::HERO)
		{
			const gmActor::HeroState& h = engine.actor_store().hero(next);
			const std::string& loc      = h.common.area_id;

			// Check if we can attack here
			bool has_enemy = false;
			for (const auto& kv : engine.actor_store().monster_instances())
			{
				if (!kv.second.common.removed &&
				    kv.second.common.area_id == loc)
				{
					has_enemy = true;
					break;
				}
			}

			if (has_enemy)
			{
				engine.play_card(next, "base_colpo_secco");
				// DAMAGE is deferred: resolve the pending attack (TAKE, no
				// reduction) so the damage is actually applied and the
				// hero's turn completes (end_hero_turn only runs inside
				// resolve_reaction for a deferred-damage card).
				if (engine.has_pending_attack())
				{
					engine.resolve_reaction(
						engine.pending_attack().defender_id,
						eldhom::DefenseReaction::TAKE);
				}
			}
			else
			{
				// Move toward monsters
				// Simple heuristic: if in ingresso move to corridoio, if in corridoio move to sala
				std::string dest;
				if (loc == "ingresso")     { dest = "corridoio"; }
				else if (loc == "corridoio") { dest = "sala"; }
				else                       { dest = "corridoio"; }

				eldhom::ActionResult r = engine.do_simple_action(
					next, eldhom::SimpleActionType::MOVE, dest);
				if (!r.ok())
				{
					// Can't move, just attack in current loc
					engine.do_simple_action(next, eldhom::SimpleActionType::RECOVER);
				}
			}
		}
		else if (kind == gmActor::ActorKind::MONSTER_GROUP)
		{
			engine.resolve_next_group_turn();
		}
		else
		{
			break;
		}
	}

	check(!victory_events.empty() || engine.mission_outcome() == eldhom::MissionOutcome::VICTORY,
	      "Mission ended in VICTORY (all monsters eliminated)");
	check(steps < 30, "Mission completed within 30 steps");
}

void test_defeat_time_limit()
{
	std::cout << "\n=== test_defeat_time_limit ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// Set time limit to 10 ticks for quick defeat
	def.defeat_conditions.clear();
	eldhom::DefeatCondition dc;
	dc.type      = "TIME_LIMIT";
	dc.threshold = 10;
	def.defeat_conditions.push_back(dc);
	// Remove ALL_PG_KO to avoid early defeat
	// (monsters can't reach PGs in ingresso if we keep them away)

	std::vector<std::string> defeat_events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&defeat_events](const eldhom::EventType& t,
		                 const std::string&,
		                 const std::string&) {
			if (t == eldhom::EVT_MISSION_DEFEAT) { defeat_events.push_back(t); }
		});

	// Do 5 RECOVER actions (cost 3 each) on Thael to advance time > 10
	for (int i = 0; i < 4 && !engine.is_over(); ++i)
	{
		const std::string& next = engine.next_actor();
		if (engine.next_actor_kind() == gmActor::ActorKind::HERO)
		{
			engine.do_simple_action(next, eldhom::SimpleActionType::RECOVER);
		}
		else
		{
			engine.resolve_next_group_turn();
		}
	}

	check(!defeat_events.empty() || engine.mission_outcome() == eldhom::MissionOutcome::DEFEAT,
	      "Mission ended in DEFEAT (time limit exceeded)");
}

void test_zone_door_blocks_monster_until_pg_crosses()
{
	std::cout << "\n=== test_zone_door_blocks_monster_until_pg_crosses ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Move whichever hero is next into corridoio, twice, so both heroes end
	// up there (opens the ingresso<->corridoio door; irrelevant to this test).
	for (int i = 0; i < 2; ++i)
	{
		const std::string next = engine.next_actor();
		check(engine.do_simple_action(next, eldhom::SimpleActionType::MOVE, "corridoio").ok(),
		      next + " moves to corridoio");
	}

	// briganti_B starts in "sala" (a different zone from "corridoio"): the
	// "sala"<->"corridoio" door is still closed, so briganti_B must NOT be
	// able to cross it even though PGs are waiting right next door.
	engine.resolve_group_turn_for("briganti_B");
	const gmActor::MonsterInstanceState& bb1_before =
		engine.actor_store().monster_instance("brigante_B1");
	check(bb1_before.common.area_id == "sala",
	      "briganti_B stays in sala while the zone door is still closed");

	// Whichever hero is next crosses into sala (opens the zone door), then
	// walks back to corridoio once it is their turn again (the other hero
	// may take filler RECOVER turns in between; turn order does not matter
	// for this test since both heroes stay put in corridoio/sala).
	const std::string crosser = engine.next_actor();
	check(engine.do_simple_action(crosser, eldhom::SimpleActionType::MOVE, "sala").ok(),
	      crosser + " crosses into sala (opens the zone door)");

	int guard = 0;
	while (engine.next_actor() != crosser && guard < 20)
	{
		if (engine.next_actor_kind() == gmActor::ActorKind::HERO)
		{
			engine.do_simple_action(engine.next_actor(), eldhom::SimpleActionType::RECOVER);
		}
		else if (engine.next_actor_kind() == gmActor::ActorKind::MONSTER_GROUP)
		{
			// Whichever group activates early finds its target still in
			// "sala" (crosser hasn't walked back yet) and simply stays put
			// ("already in contact"), which does not affect this test.
			engine.resolve_next_group_turn();
		}
		else
		{
			break;
		}
		++guard;
	}
	check(engine.do_simple_action(crosser, eldhom::SimpleActionType::MOVE, "corridoio").ok(),
	      crosser + " walks back to corridoio");

	// Now that the door is open, briganti_B should be able to cross it.
	engine.resolve_group_turn_for("briganti_B");
	const gmActor::MonsterInstanceState& bb1_after =
		engine.actor_store().monster_instance("brigante_B1");
	check(bb1_after.common.area_id == "corridoio",
	      "briganti_B crosses into corridoio once the zone door is open");
}

void test_phase0_requires_frontline()
{
	std::cout << "\n=== test_phase0_requires_frontline ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// A card that requires the caster to be in FRONTLINE (Fendente Pesante style).
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 2;
		eldhom::EldhomCard card =
			make_card("test_requires_frontline", "Test Fendente",
			          gmAlea::CardType::SINGLE, 3, { eff });
		card.requires_frontline = true;
		card_cat["test_requires_frontline"] = card;
	}
	// A card to force the caster into BACKLINE for the negative check.
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "FORMATION_PUSH";
		eff.value       = "BACKLINE";
		card_cat["test_push_backline"] =
			make_card("test_push_backline", "Test Push Backline",
			          gmAlea::CardType::SINGLE, 1, { eff });
	}

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Thael starts in FRONTLINE (Sim 01 fixture): the card must succeed.
	eldhom::ActionResult r1 = engine.play_card("thael", "test_requires_frontline");
	check(r1.ok(), "FRONTLINE caster can play a requires_frontline card");

	// Move Velyr (starts BACKLINE) to try the same card: must fail.
	eldhom::ActionResult r2 = engine.play_card("velyr", "test_requires_frontline");
	check(r2.code == eldhom::ActionResultCode::ERR_POSITION_REQUIRED,
	      "BACKLINE caster is rejected with ERR_POSITION_REQUIRED");
}

// ─────────────────────────────────────────────────────────────────────────────
// Fase 1 / Fase 2 — Carte Base (see GAME/Eldhom/info/PLAN_carte_base_e_regole.md)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * @brief Same as build_mission_01() but with a populated mission_deck (and
 * hand_limit=4) for both heroes, so hand-tracking (_hand_states) is active.
 * Needed by any test that must observe discard/draw effects
 * (DISCARD_THEN_DRAW, Colpo Secco's conditional bonus) or an eligible
 * INSTANT card (Assestarsi), since eligible_instants() only scans
 * _hand_states. Monster groups' start_timeline is pushed far into the
 * future so they never interfere with next_actor() ordering in these
 * targeted tests; call resolve_group_turn_for(group_id) explicitly when a
 * monster action is actually needed.
 *
 * IMPORTANT: thael_deck/velyr_deck must have MORE entries than hand_limit
 * (4), so the draw pile is never fully emptied by the initial deal. If the
 * deck empties, end_hero_turn()'s automatic discard->deck reshuffle-and-draw
 * silently pulls cards straight back out of the discard pile, making
 * discard_count() unusable for verifying DISCARD_THEN_DRAW effects. Passing
 * several repeated copies of a single card id also keeps the initial hand
 * deterministic (no dependency on shuffle order).
 */
eldhom::MissionDefinition build_mission_01_with_decks(
	const std::vector<eldhom::CardId>& thael_deck,
	const std::vector<eldhom::CardId>& velyr_deck)
{
	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::PgEntry& pg : def.pg_roster)
	{
		if (pg.hero_id == "thael")      { pg.mission_deck = thael_deck; }
		else if (pg.hero_id == "velyr") { pg.mission_deck = velyr_deck; }
		pg.hand_limit = 4;
	}
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }
	return def;
}

} // anonymous namespace

void test_fase1_colpo_apertura()
{
	std::cout << "\n=== test_fase1_colpo_apertura ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// base_colpo_d_apertura: SEQ_START, 2⌛, DAMAGE 1 nearest enemy
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "ENEMY_FRONTLINE";
		card_cat["base_colpo_d_apertura"] =
			make_card("base_colpo_d_apertura", "Colpo d'Apertura",
			          gmAlea::CardType::SEQ_START, 2, { eff });
	}

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	check(engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio").ok(),
	      "Thael moves to corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	eldhom::ActionResult r = engine.play_card("thael", "base_colpo_d_apertura");
	check(r.ok(), "Thael plays base_colpo_d_apertura OK");
	check(engine.has_pending_attack(), "DAMAGE effect opened a pending attack");

	eldhom::ReactionResolution res;
	engine.resolve_reaction("brigante_A1", eldhom::DefenseReaction::TAKE, &res);
	check(res.final_damage == 1, "Colpo d'Apertura deals 1 damage");

	const gmAlea::SequenceState& seq = engine.sequence_state("thael");
	check(seq.active, "Sequence is active after Colpo d'Apertura (SEQ_START)");
}

void test_fase1_passo_e_lama()
{
	std::cout << "\n=== test_fase1_passo_e_lama ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// base_passo_e_lama: SEQ_START, 3⌛, MOVE 1 + DAMAGE 1
	{
		eldhom::EldhomEffect move_eff;
		move_eff.effect_type = "MOVE";
		move_eff.amount      = 1;
		move_eff.target      = "PLAYER_CHOICE";

		eldhom::EldhomEffect dmg_eff;
		dmg_eff.effect_type = "DAMAGE";
		dmg_eff.amount      = 1;
		dmg_eff.target      = "ENEMY_FRONTLINE";

		card_cat["base_passo_e_lama"] =
			make_card("base_passo_e_lama", "Passo e Lama",
			          gmAlea::CardType::SEQ_START, 3, { move_eff, dmg_eff });
	}

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Thael starts in "ingresso"; play the card with destination "corridoio"
	// (1 hop, where brigante_A1 waits) — should move AND attack in one turn.
	eldhom::ActionResult r = engine.play_card("thael", "base_passo_e_lama", "corridoio");
	check(r.ok(), "Thael plays base_passo_e_lama OK");
	check(engine.actor_store().hero("thael").common.area_id == "corridoio",
	      "Passo e Lama moved Thael to corridoio");
	check(engine.has_pending_attack(), "Passo e Lama's DAMAGE effect opened a pending attack");

	eldhom::ReactionResolution res;
	engine.resolve_reaction("brigante_A1", eldhom::DefenseReaction::TAKE, &res);
	check(res.final_damage == 1, "Passo e Lama deals 1 damage after moving");
}

void test_fase1_mano_ferma()
{
	std::cout << "\n=== test_fase1_mano_ferma ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }

	eldhom::SpecialObject lever;
	lever.object_id       = "leva_test";
	lever.name            = "Leva di Prova";
	lever.type            = "LEVER";
	lever.location_id     = "sala";
	def.special_objects   = { lever };

	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// base_mano_ferma: SINGLE, 2⌛, INTERACT
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "INTERACT";
		card_cat["base_mano_ferma"] =
			make_card("base_mano_ferma", "Mano Ferma",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	std::vector<std::string> events;
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat,
		[&events](const eldhom::EventType& t, const std::string&, const std::string&) {
			events.push_back(t);
		});

	// Move Thael to sala (ingresso -> corridoio -> sala); Velyr fills turns.
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "sala");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	events.clear();
	eldhom::ActionResult r = engine.play_card("thael", "base_mano_ferma");
	check(r.ok(), "Thael plays base_mano_ferma OK");

	bool porta_aperta = false;
	for (const std::string& ev : events)
	{
		if (ev == eldhom::EVT_PORTA_APERTA) { porta_aperta = true; break; }
	}
	check(porta_aperta, "Mano Ferma triggers the LEVER special object (INTERACT)");
}

void test_fase1_passo_cauto_scatto_breve_evita_nemici()
{
	std::cout << "\n=== test_fase1_passo_cauto_scatto_breve_evita_nemici ===\n";

	auto make_engine = []() {
		eldhom::MissionDefinition def = build_mission_01();
		for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }
		auto card_cat     = build_card_catalog();
		auto behavior_cat = build_behavior_catalog();

		eldhom::EldhomEffect passo_cauto_eff;
		passo_cauto_eff.effect_type           = "MOVE";
		passo_cauto_eff.amount                = 2;
		passo_cauto_eff.target                = "PLAYER_CHOICE";
		passo_cauto_eff.avoid_enemy_locations = true;
		card_cat["base_passo_cauto"] =
			make_card("base_passo_cauto", "Passo Cauto",
			          gmAlea::CardType::SINGLE, 2, { passo_cauto_eff });

		eldhom::EldhomEffect scatto_breve_eff;
		scatto_breve_eff.effect_type           = "MOVE";
		scatto_breve_eff.amount                = 3;
		scatto_breve_eff.target                = "PLAYER_CHOICE";
		scatto_breve_eff.avoid_enemy_locations = true;
		card_cat["base_scatto_breve"] =
			make_card("base_scatto_breve", "Scatto Breve",
			          gmAlea::CardType::SINGLE, 2, { scatto_breve_eff });

		return eldhom::EldhomEngine::from_definition(def, card_cat, behavior_cat, nullptr);
	};

	// "corridoio" holds brigante_A1 (FRONTLINE) and is the only path from
	// "ingresso" to "sala" (linear graph).

	{
		eldhom::EldhomEngine engine = make_engine();
		eldhom::ActionResult r = engine.play_card("thael", "base_passo_cauto", "sala");
		check(r.ok(), "Passo Cauto to sala: playing the card itself does not error");
		check(engine.actor_store().hero("thael").common.area_id == "ingresso",
		      "Passo Cauto cannot cross corridoio (enemy-occupied intermediate step)");
	}
	{
		eldhom::EldhomEngine engine = make_engine();
		engine.play_card("thael", "base_passo_cauto", "corridoio");
		check(engine.actor_store().hero("thael").common.area_id == "corridoio",
		      "Passo Cauto CAN end in corridoio (enemy-occupied final destination)");
	}
	{
		eldhom::EldhomEngine engine = make_engine();
		engine.play_card("thael", "base_scatto_breve", "sala");
		check(engine.actor_store().hero("thael").common.area_id == "ingresso",
		      "Scatto Breve cannot cross corridoio (enemy-occupied intermediate step)");
	}
}

void test_fase1_colpo_secco_bonus_condizionale()
{
	std::cout << "\n=== test_fase1_colpo_secco_bonus_condizionale ===\n";

	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEffect dmg_eff;
	dmg_eff.effect_type = "DAMAGE";
	dmg_eff.amount      = 1;
	dmg_eff.target      = "ENEMY_FRONTLINE";

	eldhom::EldhomEffect bonus_eff;
	bonus_eff.effect_type = "DISCARD_THEN_DRAW";
	bonus_eff.amount      = 1;
	bonus_eff.target      = "SELF";
	bonus_eff.condition   = "IF_BOTH_FRONTLINE";

	card_cat["base_colpo_secco"] =
		make_card("base_colpo_secco", "Colpo Secco",
		          gmAlea::CardType::SINGLE, 2, { dmg_eff, bonus_eff });

	// Scenario A: attacker (FRONTLINE) and target (FRONTLINE) — bonus applies.
	{
		eldhom::MissionDefinition def = build_mission_01_with_decks(
			std::vector<eldhom::CardId>(6, "base_colpo_secco"),
			std::vector<eldhom::CardId>(4, "base_riprendere_fiato"));
		eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
			def, card_cat, behavior_cat, nullptr);

		engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
		engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

		const std::vector<eldhom::CardId>& hand = engine.hand_cards("thael");
		check(!hand.empty(), "Thael has cards in hand before Colpo Secco");
		const int discard_before = engine.discard_count("thael");

		// DAMAGE is deferred (parks a pending attack and returns immediately,
		// WITHOUT calling end_hero_turn), so discard_count() here reflects only
		// the conditional bonus — not yet the end-of-turn played->discard flush.
		eldhom::ActionResult r =
			engine.play_card("thael", "base_colpo_secco", "", { hand.front() });
		check(r.ok(), "Thael (FRONTLINE) plays base_colpo_secco OK vs FRONTLINE target");

		check(engine.discard_count("thael") == discard_before + 1,
		      "Bonus applied: 1 card discarded (attacker and target both FRONTLINE)");

		engine.resolve_reaction("brigante_A1", eldhom::DefenseReaction::TAKE);
	}

	// Scenario B: attacker pushed to BACKLINE — bonus must NOT apply, even
	// though the target is still FRONTLINE.
	{
		auto card_cat_b = card_cat;
		eldhom::EldhomEffect push_eff;
		push_eff.effect_type = "FORMATION_PUSH";
		push_eff.value       = "BACKLINE";
		card_cat_b["test_push_backline_cs"] =
			make_card("test_push_backline_cs", "Test Push Backline",
			          gmAlea::CardType::SINGLE, 1, { push_eff });

		eldhom::MissionDefinition def = build_mission_01_with_decks(
			std::vector<eldhom::CardId>(6, "base_colpo_secco"),
			std::vector<eldhom::CardId>(4, "base_riprendere_fiato"));
		for (eldhom::PgEntry& pg : def.pg_roster)
		{
			if (pg.hero_id != "thael") { continue; }
			// Deliberately hand_limit == mission_deck.size() here (unlike the
			// shared helper's default) so test_push_backline_cs is GUARANTEED
			// to be in the initial hand regardless of shuffle order.
			pg.mission_deck.push_back("test_push_backline_cs");
			pg.hand_limit = static_cast<int>(pg.mission_deck.size());
		}
		eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
			def, card_cat_b, behavior_cat, nullptr);

		engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
		engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);
		eldhom::ActionResult rpush = engine.play_card("thael", "test_push_backline_cs");
		check(rpush.ok(), "Thael plays test_push_backline_cs OK");
		engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

		check(engine.actor_store().hero("thael").common.area_position ==
		      gmActor::AreaPosition::BACKLINE, "Thael pushed to BACKLINE for scenario B");

		const std::vector<eldhom::CardId>& hand = engine.hand_cards("thael");
		const int discard_before = engine.discard_count("thael");

		eldhom::ActionResult r =
			engine.play_card("thael", "base_colpo_secco", "", { hand.front() });
		check(r.ok(), "Thael (BACKLINE) plays base_colpo_secco OK vs FRONTLINE target");

		check(engine.discard_count("thael") == discard_before,
		      "Bonus NOT applied when attacker is BACKLINE");

		engine.resolve_reaction("brigante_A1", eldhom::DefenseReaction::TAKE);
	}
}

void test_fase1_riprendere_fiato()
{
	std::cout << "\n=== test_fase1_riprendere_fiato ===\n";

	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	eldhom::EldhomEffect heal_eff;
	heal_eff.effect_type = "HEAL";
	heal_eff.amount      = 1;
	heal_eff.target      = "SELF";

	eldhom::EldhomEffect discard_draw_eff;
	discard_draw_eff.effect_type = "DISCARD_THEN_DRAW";
	discard_draw_eff.amount      = 1;
	discard_draw_eff.target      = "SELF";

	card_cat["base_riprendere_fiato"] =
		make_card("base_riprendere_fiato", "Riprendere Fiato",
		          gmAlea::CardType::SINGLE, 3, { heal_eff, discard_draw_eff });

	eldhom::MissionDefinition def = build_mission_01_with_decks(
		std::vector<eldhom::CardId>(8, "base_riprendere_fiato"),
		std::vector<eldhom::CardId>(8, "base_riprendere_fiato"));
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Move Thael (FRONTLINE) next to brigante_A1 and take a real hit so HEAL
	// is observable (current_hp < max_hp).
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	const int hp_before_hit = engine.actor_store().hero("thael").common.current_hp;
	engine.resolve_group_turn_for("briganti_A"); // brigante_assalto: deals damage directly
	const int hp_after_hit = engine.actor_store().hero("thael").common.current_hp;
	check(hp_after_hit < hp_before_hit, "Thael took real damage from briganti_A (setup)");

	const std::vector<eldhom::CardId>& hand = engine.hand_cards("thael");
	check(!hand.empty(), "Thael has cards in hand before Riprendere Fiato");
	const int discard_before_thael = engine.discard_count("thael");

	eldhom::ActionResult r =
		engine.play_card("thael", "base_riprendere_fiato", "", { hand.front() });
	check(r.ok(), "Thael (FRONTLINE) plays base_riprendere_fiato OK");
	check(engine.actor_store().hero("thael").common.current_hp == hp_after_hit + 1,
	      "Riprendere Fiato heals 1 HP");
	// +1 for the discarded card, +1 for base_riprendere_fiato itself moving
	// from played to discard when end_hero_turn runs.
	check(engine.discard_count("thael") == discard_before_thael + 2,
	      "Riprendere Fiato discards 1 card (FRONTLINE: base amount, not doubled)");

	// Velyr (BACKLINE): discard/draw amount doubles to 2.
	const std::vector<eldhom::CardId>& hand_v = engine.hand_cards("velyr");
	check(hand_v.size() >= 2, "Velyr has at least 2 cards in hand");
	const int discard_before_velyr = engine.discard_count("velyr");

	eldhom::ActionResult rv = engine.play_card(
		"velyr", "base_riprendere_fiato", "", { hand_v[0], hand_v[1] });
	check(rv.ok(), "Velyr (BACKLINE) plays base_riprendere_fiato OK");
	// +2 for the discarded cards (doubled, BACKLINE), +1 for the card itself.
	check(engine.discard_count("velyr") == discard_before_velyr + 3,
	      "Riprendere Fiato discards 2 cards for a BACKLINE caster (doubled)");
}

void test_fase2_assestarsi()
{
	std::cout << "\n=== test_fase2_assestarsi ===\n";

	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	// base_assestarsi: INSTANT, 1⌛, reacts to EVT_ENEMY_APPROACH, pushes
	// caster to FRONTLINE.
	eldhom::EldhomEffect push_eff;
	push_eff.effect_type = "FORMATION_PUSH";
	push_eff.value       = "FRONTLINE";

	eldhom::EldhomCard assestarsi =
		make_card("base_assestarsi", "Assestarsi", gmAlea::CardType::INSTANT, 1, { push_eff });
	assestarsi.reaction_trigger = eldhom::EVT_ENEMY_APPROACH;
	card_cat["base_assestarsi"] = assestarsi;

	eldhom::MissionDefinition def = build_mission_01_with_decks(
		std::vector<eldhom::CardId>(4, "base_riprendere_fiato"),
		std::vector<eldhom::CardId>(8, "base_assestarsi"));
	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	check(!engine.has_pending_reactive_window(), "No reactive window open initially");
	check(engine.actor_store().hero("velyr").common.area_position ==
	      gmActor::AreaPosition::BACKLINE, "Velyr starts in BACKLINE");

	// "ingresso" and "corridoio" are different zones (no shared numeric
	// suffix): the zone-boundary door starts CLOSED and monsters cannot cross
	// it until a PG has opened it by walking through. Open it first, then
	// walk Thael back, so briganti_A is free to advance into "ingresso".
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);
	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "ingresso");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	// briganti_A starts in "corridoio", adjacent to "ingresso" where both
	// heroes stand. Force its group turn explicitly (bypassing next_actor
	// ordering, since build_mission_01_with_decks() pushed its timeline far
	// out) so it moves into "ingresso" via its behavior deck (brigante_assalto
	// starts with an optional MOVE_TOWARD_PG step).
	engine.resolve_group_turn_for("briganti_A");

	check(engine.has_pending_reactive_window(), "Enemy approach opened a reactive window");
	check(engine.pending_reactive_window().trigger == eldhom::EVT_ENEMY_APPROACH,
	      "Reactive window trigger is EVT_ENEMY_APPROACH");

	eldhom::ActionResult r =
		engine.play_reactive_instants({ { "velyr", "base_assestarsi" } });
	check(r.ok(), "Velyr plays base_assestarsi as a reactive instant");
	check(engine.actor_store().hero("velyr").common.area_position ==
	      gmActor::AreaPosition::FRONTLINE, "Assestarsi pushed Velyr to FRONTLINE");
	check(!engine.has_pending_reactive_window(), "Reactive window closed after playing Assestarsi");
}

void test_fase2_pressione_continua()
{
	std::cout << "\n=== test_fase2_pressione_continua ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();

	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "ENEMY_FRONTLINE";
		card_cat["base_colpo_d_apertura"] =
			make_card("base_colpo_d_apertura", "Colpo d'Apertura",
			          gmAlea::CardType::SEQ_START, 2, { eff });
	}
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "ENEMY_FRONTLINE";
		card_cat["base_pressione_continua"] =
			make_card("base_pressione_continua", "Pressione Continua",
			          gmAlea::CardType::SEQ_CONTINUE, 2, { eff });
	}

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	engine.play_card("thael", "base_colpo_d_apertura");
	eldhom::ReactionResolution res1;
	engine.resolve_reaction("brigante_A1", eldhom::DefenseReaction::TAKE, &res1);
	check(engine.sequence_state("thael").active, "Sequence active after Colpo d'Apertura");
	check(res1.defender_hp_after == 2, "brigante_A1 HP is 2 after Colpo d'Apertura (3-1)");

	check(engine.next_actor() == "thael", "Thael still next during sequence");

	engine.play_card("thael", "base_pressione_continua");
	eldhom::ReactionResolution res2;
	engine.resolve_reaction("brigante_A1", eldhom::DefenseReaction::TAKE, &res2);
	check(res2.defender_hp_after == 1, "brigante_A1 HP is 1 after Pressione Continua (2-1)");
	check(engine.sequence_state("thael").active,
	      "Sequence still active after Pressione Continua (SEQ_CONTINUE keeps it open)");
}

// ─────────────────────────────────────────────────────────────────────────────
// Explicit player-chosen target for card DAMAGE effects (mirrors
// declare_attack's targeting for Attacco Semplice; see EldhomRuleAdapter::
// is_valid_target_in_range).
// ─────────────────────────────────────────────────────────────────────────────

void test_card_explicit_target_overrides_auto_select()
{
	std::cout << "\n=== test_card_explicit_target_overrides_auto_select ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }

	// Add a second FRONTLINE enemy in "corridoio" with MORE hp than
	// brigante_A1 (3 hp), so automatic nearest-target selection (which
	// prefers the lowest-hp candidate) would always pick brigante_A1 —
	// letting us prove an explicit target_id actually overrides that choice.
	for (eldhom::MonsterGroupEntry& g : def.monster_groups)
	{
		if (g.group_id != "briganti_A") { continue; }
		eldhom::MonsterInstanceEntry extra;
		extra.instance_id = "brigante_A3";
		extra.position    = "FRONTLINE";
		extra.max_hp      = 9;
		extra.damage      = 1;
		extra.movement    = 2;
		g.instances.push_back(extra);
	}

	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "ENEMY_FRONTLINE";
		card_cat["base_colpo_secco"] =
			make_card("base_colpo_secco", "Colpo Secco",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	// Explicitly target brigante_A3 (9 hp) instead of the auto-selected
	// brigante_A1 (3 hp, lower — would win nearest_target's HP tie-break).
	eldhom::ActionResult r = engine.play_card(
		"thael", "base_colpo_secco", "", {}, "brigante_A3");
	check(r.ok(), "Thael plays base_colpo_secco with explicit target_id OK");
	check(engine.has_pending_attack(), "Explicit target opened a pending attack");
	check(engine.pending_attack().defender_id == "brigante_A3",
	      "Pending attack defender is the EXPLICITLY chosen target, not the nearest/lowest-hp one");

	eldhom::ReactionResolution res;
	engine.resolve_reaction("brigante_A3", eldhom::DefenseReaction::TAKE, &res);
	check(res.final_damage == 1, "Damage applied to the explicitly chosen target");
	check(res.defender_hp_after == 8, "brigante_A3 HP reduced (9-1=8), brigante_A1 untouched");
}

void test_card_explicit_target_rejected_when_invalid()
{
	std::cout << "\n=== test_card_explicit_target_rejected_when_invalid ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "ENEMY_FRONTLINE";
		card_cat["base_colpo_secco"] =
			make_card("base_colpo_secco", "Colpo Secco",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	// Thael stays in "ingresso": brigante_B1 exists but sits in "sala",
	// out of mischia (range 0) reach — an invalid target for this card.
	const int timeline_before = engine.timeline_position("thael");

	eldhom::ActionResult r = engine.play_card(
		"thael", "base_colpo_secco", "", {}, "brigante_B1");
	check(r.code == eldhom::ActionResultCode::ERR_NO_VALID_TARGET,
	      "Invalid/out-of-range target_id is rejected with ERR_NO_VALID_TARGET");
	check(!engine.has_pending_attack(), "No pending attack was opened for a rejected target");
	check(engine.timeline_position("thael") == timeline_before,
	      "Action is atomic: timeline cost NOT charged when the target is rejected");
	check(engine.next_actor() == "thael",
	      "Thael's turn did not advance/end after the rejected target_id");
}

void test_card_no_target_falls_back_to_auto_select()
{
	std::cout << "\n=== test_card_no_target_falls_back_to_auto_select ===\n";

	eldhom::MissionDefinition def = build_mission_01();
	for (eldhom::MonsterGroupEntry& g : def.monster_groups) { g.start_timeline = 1000; }
	auto card_cat     = build_card_catalog();
	auto behavior_cat = build_behavior_catalog();
	{
		eldhom::EldhomEffect eff;
		eff.effect_type = "DAMAGE";
		eff.amount      = 1;
		eff.target      = "ENEMY_FRONTLINE";
		card_cat["base_colpo_secco"] =
			make_card("base_colpo_secco", "Colpo Secco",
			          gmAlea::CardType::SINGLE, 2, { eff });
	}

	eldhom::EldhomEngine engine = eldhom::EldhomEngine::from_definition(
		def, card_cat, behavior_cat, nullptr);

	engine.do_simple_action("thael", eldhom::SimpleActionType::MOVE, "corridoio");
	engine.do_simple_action("velyr", eldhom::SimpleActionType::RECOVER);

	// No target_id supplied (default empty): falls back to automatic
	// nearest-target selection — unchanged, pre-existing behaviour.
	eldhom::ActionResult r = engine.play_card("thael", "base_colpo_secco");
	check(r.ok(), "Thael plays base_colpo_secco without a target_id OK");
	check(engine.has_pending_attack(), "Auto-select opened a pending attack");
	check(engine.pending_attack().defender_id == "brigante_A1",
	      "Auto-select still picks brigante_A1 (the only/nearest valid target)");
}



int main()
{
	std::cout << "Le Pergamene di Eldhom — test_eldhom_mission01\n";
	std::cout << "================================================\n";

	test_initial_state();
	test_pg_turn_simple_action_move();
	test_pg_turn_not_your_turn();
	test_pg_turn_play_card_single();
	test_pg_turn_sequence();
	test_monster_group_turn();
	test_victory_all_monsters_eliminated();
	test_defeat_time_limit();
	test_zone_door_blocks_monster_until_pg_crosses();
	test_phase0_requires_frontline();

	test_fase1_colpo_apertura();
	test_fase1_passo_e_lama();
	test_fase1_mano_ferma();
	test_fase1_passo_cauto_scatto_breve_evita_nemici();
	test_fase1_colpo_secco_bonus_condizionale();
	test_fase1_riprendere_fiato();
	test_fase2_assestarsi();
	test_fase2_pressione_continua();

	test_card_explicit_target_overrides_auto_select();
	test_card_explicit_target_rejected_when_invalid();
	test_card_no_target_falls_back_to_auto_select();

	std::cout << "\n================================================\n";
	std::cout << "PASS: " << s_pass << "   FAIL: " << s_fail << "\n";

	return s_fail == 0 ? 0 : 1;
}
