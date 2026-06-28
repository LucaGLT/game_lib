#ifndef ELDHOM_ENGINE_ELDHOMENGINE_HPP
#define ELDHOM_ENGINE_ELDHOMENGINE_HPP

/**
 * @file engine/EldhomEngine.hpp
 * @brief Main orchestrator for Le Pergamene di Eldhôm game engine.
 *
 * `EldhomEngine` manages one complete mission session.  It owns:
 * - the `gmActor::ActorStore` with all PG and monster state
 * - per-hero `gmAlea::SequenceState` maps
 * - the monster behavior deck states (via `EldhomBehaviorAdapter`)
 * - all stateless service objects (SequenceAdapter, FormationAdapter,
 *   RuleAdapter, TargetingFilter, MissionEventSystem)
 *
 * ### Turn loop (§2 — Ordine di Iniziativa)
 *
 * The engine does NOT run an autonomous game loop.  The caller (test harness,
 * mock server, or GUI) drives it step by step:
 *
 * 1. Call `next_actor()` to identify who acts.
 * 2. If `next_actor_kind() == HERO`, call `do_simple_action()` or `play_card()`
 *    until `end_hero_turn()` is called or the card is turn-ending.
 * 3. If `next_actor_kind() == MONSTER_GROUP`, call `resolve_next_group_turn()`.
 * 4. Repeat until `is_over()` returns true.
 *
 * ### Initialization
 *
 * Use `EldhomEngine::from_definition(def, card_catalog, behavior_catalog)`
 * to construct a fully initialised engine from a `MissionDefinition`.
 *
 * ### Events
 *
 * Register an event callback via the constructor or `set_event_callback()`.
 * The engine emits events using the string constants in `EldhomTypes.hpp`.
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"
#include "GAME/Eldhom/CoreEngine/engine/CardData.hpp"
#include "GAME/Eldhom/CoreEngine/sequence/EldhomSequenceAdapter.hpp"
#include "GAME/Eldhom/CoreEngine/formation/EldhomFormationAdapter.hpp"
#include "GAME/Eldhom/CoreEngine/monsters/EldhomBehaviorAdapter.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionDefinition.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionEventSystem.hpp"
#include "GAME/Eldhom/CoreEngine/targeting/TargetingFilter.hpp"
#include "GAME/Eldhom/CoreEngine/rules/EldhomRuleAdapter.hpp"

#include "gmActor/actors/ActorStore.hpp"
#include "gmActor/behavior/BehaviorCardProcessor.hpp"   // gmActor::StepExecutor
#include "gmAlea/SequenceState.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace eldhom {

/**
 * @brief Callback type for engine events.
 *
 * Parameters:
 * - event_type : string constant from EldhomTypes.hpp
 * - actor_id   : actor involved (may be empty)
 * - payload    : event-specific data string (may be empty)
 */
using EngineEventCallback =
	std::function<void(const EventType&, const std::string& actor_id,
	                   const std::string& payload)>;

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct HeroHandState
 * @brief Per-hero hand and deck tracking state.
 *
 * Tracks the shuffled draw pile, the current hand, and the discard pile.
 * Managed internally by EldhomEngine; accessed via hand_cards().
 */
struct HeroHandState
{
	std::vector<CardId> hand;    ///< Cards currently in the hero's hand
	std::vector<CardId> deck;    ///< Shuffled draw pile (top = back())
	std::vector<CardId> discard; ///< Played / discarded cards
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct PendingAttack
 * @brief Engine-side state held while an interactive reaction window is open.
 *
 * Between `declare_attack()` and `resolve_reaction()` the engine parks the
 * attack here.  No effect is applied and the attacker's turn does not advance
 * until the defender's reaction is resolved.  `active == false` means no
 * reaction window is open and normal commands are accepted.
 */
struct PendingAttack
{
	bool             active      = false; ///< True while the reaction window is open
	HeroId           attacker_id;         ///< Hero that declared the attack
	gmActor::ActorId defender_id;         ///< Target that must react
	int              base_damage = 0;     ///< Declared (pre-reaction) damage
	int              attack_cost = 0;     ///< Timeline cost charged on resolution
	std::string      source;              ///< "simple" or the source card id
};

/**
 * @struct ReactionResolution
 * @brief Outcome of `resolve_reaction()`, used by callers to build events.
 */
struct ReactionResolution
{
	bool             ok           = false; ///< True if the reaction was resolved
	HeroId           attacker_id;          ///< Hero that attacked
	gmActor::ActorId defender_id;          ///< Target that reacted
	int              base_damage  = 0;     ///< Pre-reaction damage
	int              final_damage = 0;     ///< Damage actually applied
	DefenseReaction  reaction     = DefenseReaction::TAKE; ///< Chosen reaction
	int              defender_hp_after = 0;///< Defender HP after the attack
	bool             defender_ko  = false; ///< True if the defender reached 0 HP
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class EldhomEngine
 * @brief Main game-engine orchestrator for Le Pergamene di Eldhôm.
 */
class EldhomEngine
{
public:
	// ── Construction / initialisation ─────────────────────────────────────────

	/**
	 * @brief Constructs an engine from a pre-built component set.
	 *
	 * Prefer `from_definition()` for normal use.
	 *
	 * @param store          Fully populated actor store.
	 * @param seq_states     Per-hero sequence states (keyed by hero actor_id).
	 * @param card_catalog   Hero action card catalog.
	 * @param hero_factions  Set of hero faction IDs (used for targeting).
	 * @param monster_factions Set of monster faction IDs.
	 * @param adjacency      Location adjacency map.
	 * @param behavior_adapter Fully configured behavior adapter.
	 * @param event_system   Fully configured mission event system.
	 * @param on_event       Engine event callback.
	 */
	EldhomEngine(
		gmActor::ActorStore                                              store,
		std::unordered_map<HeroId, gmAlea::SequenceState>               seq_states,
		std::unordered_map<CardId, EldhomCard>                          card_catalog,
		std::vector<std::string>                                        hero_factions,
		std::vector<std::string>                                        monster_factions,
		std::unordered_map<LocationId, std::vector<LocationId>>         adjacency,
		EldhomBehaviorAdapter                                           behavior_adapter,
		MissionEventSystem                                              event_system,
		EngineEventCallback                                             on_event);

	/**
	 * @brief Builds a fully initialised engine from a `MissionDefinition`.
	 *
	 * @param def              Mission definition (locations, PG roster, groups).
	 * @param card_catalog     Hero card catalog.
	 * @param behavior_catalog Monster behavior card catalog.
	 * @param on_event         Engine event callback (may be nullptr).
	 * @return Fully constructed and initialised `EldhomEngine`.
	 */
	static EldhomEngine from_definition(
		const MissionDefinition&                                    def,
		const std::unordered_map<CardId, EldhomCard>&               card_catalog,
		const std::unordered_map<gmActor::CardId, gmActor::BehaviorCard>& behavior_catalog,
		EngineEventCallback                                         on_event);

	// ── Turn / actor selection ─────────────────────────────────────────────────

	/**
	 * @brief Returns the actor ID of the next actor to activate.
	 *
	 * Selects the actor with the lowest timeline_position.  Ties are
	 * broken by tie_break_rank (§2.2): heroes first, then allies, then
	 * monster groups.
	 *
	 * @return Actor ID, or empty string if all actors are removed.
	 */
	std::string next_actor() const;

	/**
	 * @brief Returns the `ActorKind` of the next actor.
	 *
	 * Returns `ActorKind::MISSION_SYSTEM` as a sentinel if no actor is found.
	 */
	gmActor::ActorKind next_actor_kind() const;

	// ── PG turn API ───────────────────────────────────────────────────────────

	/**
	 * @brief Performs a Turno PG: Azione Semplice.
	 *
	 * Validates that `hero_id` is the next actor.  Applies the simple action
	 * effect (MOVE, ATTACK, INTERACT, RECOVER) and advances the hero's
	 * timeline_position by the appropriate cost.
	 *
	 * MOVE requires `destination` to be non-empty.
	 * Other action types ignore `destination`.
	 *
	 * After the action: runs formation check and emits EVT_PG_TURN_ENDED.
	 *
	 * @param hero_id     Hero actor ID.
	 * @param action_type Action type.
	 * @param destination Target LocationId (only for MOVE).
	 * @return `ActionResult` with ok()==true on success.
	 */
	ActionResult do_simple_action(
		const HeroId&       hero_id,
		SimpleActionType    action_type,
		const LocationId&   destination = {});

	/**
	 * @brief Performs a Turno PG: gioca una Carta Azione.
	 *
	 * Validates sequence legality, applies all card effects, advances the
	 * hero's timeline_position by `card.timeline_cost`.
	 *
	 * If the card is turn-ending (SINGLE or SEQ_END), emits EVT_PG_TURN_ENDED.
	 * Otherwise the hero may play another card (SEQ_CONTINUE or INSTANT).
	 *
	 * @param hero_id  Hero actor ID.
	 * @param card_id  Card to play (must be in the hand for the hero — hand
	 *                 management is external in this prototype).
	 * @return `ActionResult`.
	 */
	ActionResult play_card(const HeroId& hero_id, const CardId& card_id);

	/**
	 * @brief Voluntarily ends a hero's sequence turn (stop_sequence).
	 *
	 * Valid only when a sequence is active and the hero decides not to play
	 * a SEQ_CONTINUE or SEQ_END card.
	 *
	 * Resets the hero's sequence state and emits EVT_PG_TURN_ENDED.
	 *
	 * @param hero_id Hero actor ID.
	 * @return `ActionResult`.
	 */
	ActionResult stop_sequence(const HeroId& hero_id);

	// ── Interactive attack / reaction window API (§5.5) ───────────────────────

	/**
	 * @brief Declares an interactive attack against an explicit target.
	 *
	 * Unlike the automatic `SimpleActionType::ATTACK`, this opens a reaction
	 * window: the engine validates the target (same location, §15 Proiezione
	 * frontline rule) and parks a `PendingAttack`.  No damage is applied and
	 * the attacker's turn does NOT advance until `resolve_reaction()` is
	 * called.  The controlling player chooses the defender's reaction.
	 *
	 * @param hero_id   Attacking hero actor ID (must be the active actor).
	 * @param target_id Target actor ID (must be a valid target in range).
	 * @return `ActionResult`.  On success, `has_pending_attack()` becomes true.
	 */
	ActionResult declare_attack(
		const HeroId&           hero_id,
		const gmActor::ActorId& target_id);

	/**
	 * @brief Resolves the open reaction window with the chosen reaction.
	 *
	 * Applies the reaction (TAKE / BLOCK / DODGE), then charges the attacker's
	 * timeline cost, runs the formation check and ends the hero's turn.
	 *
	 * @param defender_id Actor reacting (must match the pending defender).
	 * @param reaction    The chosen defensive reaction.
	 * @param out         Optional output filled with the resolution details.
	 * @return `ActionResult`.  ERR_NO_PENDING_ATTACK if no window is open.
	 */
	ActionResult resolve_reaction(
		const gmActor::ActorId& defender_id,
		DefenseReaction         reaction,
		ReactionResolution*     out = nullptr);

	/** @brief Returns true while an interactive reaction window is open. */
	bool has_pending_attack() const;

	/** @brief Read-only access to the current pending attack. */
	const PendingAttack& pending_attack() const;

	/**
	 * @brief Returns the reactions the current pending defender may choose.
	 *
	 * Always includes TAKE and BLOCK.  DODGE is offered only when the defender
	 * is on the FRONTLINE (so it has a BACKLINE to retreat to).  Returns an
	 * empty vector when no attack is pending.
	 */
	std::vector<DefenseReaction> allowed_reactions() const;

	// ── Monster group turn API ────────────────────────────────────────────────

	/**
	 * @brief Resolves the Turno Gruppo Mostri for the next monster group.
	 *
	 * Identifies the next monster group on the timeline, then calls
	 * `resolve_group_turn_for(group_id)`.
	 *
	 * @return `ActionResult`.  ERR_NOT_YOUR_TURN if next actor is not a group.
	 */
	ActionResult resolve_next_group_turn();

	/**
	 * @brief Resolves the Turno Gruppo Mostri for a specific group (§40).
	 *
	 * Steps:
	 * 1. Get active behavior card from the behavior adapter.
	 * 2. For each step in the card, call the StepExecutor for each member.
	 * 3. Advance group timeline_position by each step's timeline_cost.
	 * 4. After all steps: run formation check for affected locations.
	 * 5. Advance behavior deck (round-robin).
	 * 6. Emit EVT_GROUP_ACTIVATED.
	 *
	 * @param group_id Monster group actor ID.
	 * @return `ActionResult`.
	 */
	ActionResult resolve_group_turn_for(const GroupId& group_id);

	// ── State queries ─────────────────────────────────────────────────────────

	/** @brief Returns current mission time (⌛). */
	int mission_time() const;

	/** @brief Returns current mission outcome. */
	MissionOutcome mission_outcome() const;

	/** @brief Returns true if the mission has ended. */
	bool is_over() const;

	/**
	 * @brief Returns the current sequence state for a hero.
	 * @param hero_id Hero actor ID.
	 * @throws std::out_of_range if hero_id is not registered.
	 */
	const gmAlea::SequenceState& sequence_state(const HeroId& hero_id) const;

	/**
	 * @brief Returns the timeline position of an actor.
	 *
	 * Heroes and monster instances: read from `ActorStateCommon::timeline_position`.
	 * Monster groups: read from `MonsterGroupState::timeline_position`.
	 *
	 * @param actor_id Actor ID.
	 * @throws std::out_of_range if not found.
	 */
	int timeline_position(const std::string& actor_id) const;

	/** @brief Read-only access to the actor store. */
	const gmActor::ActorStore& actor_store() const;

	/**
	 * @brief Returns the cards currently in a hero's hand.
	 *
	 * @param hero_id  Hero actor ID.
	 * @return Const reference to the hand card list.
	 * @throws std::out_of_range if hero_id is not registered.
	 */
	const std::vector<CardId>& hand_cards(const HeroId& hero_id) const;

	/**
	 * @brief Returns the number of cards in the hero's draw pile.
	 * @param hero_id  Hero actor ID.
	 */
	int deck_count(const HeroId& hero_id) const;

	/**
	 * @brief Returns the number of cards in the hero's discard pile.
	 * @param hero_id  Hero actor ID.
	 */
	int discard_count(const HeroId& hero_id) const;

	// ── Event callback ────────────────────────────────────────────────────────

	/**
	 * @brief Replaces the event callback.
	 * @param cb New callback (nullptr disables emission).
	 */
	void set_event_callback(EngineEventCallback cb);

private:
	// ── Owned state ───────────────────────────────────────────────────────────
	gmActor::ActorStore                               _store;
	std::unordered_map<HeroId, gmAlea::SequenceState> _seq_states;
	std::unordered_map<CardId, EldhomCard>            _card_catalog;
	std::unordered_map<HeroId, HeroHandState>         _hand_states; ///< Per-hero deck/hand

	// ── Configuration ─────────────────────────────────────────────────────────
	std::vector<std::string>                                  _hero_factions;
	std::vector<std::string>                                  _monster_factions;
	std::unordered_map<LocationId, std::vector<LocationId>>   _adjacency;

	// ── Services ──────────────────────────────────────────────────────────────
	EldhomSequenceAdapter  _sequence_adapter;
	EldhomFormationAdapter _formation_adapter;
	TargetingFilter        _targeting;
	EldhomBehaviorAdapter  _behavior_adapter;
	MissionEventSystem     _mission_events;
	EldhomRuleAdapter      _rule_adapter;

	// ── Event emission ────────────────────────────────────────────────────────
	EngineEventCallback    _on_event;

	// ── Interactive attack state ──────────────────────────────────────────────
	PendingAttack          _pending; ///< Open reaction window (active==false if none)

	void emit(const EventType& type,
	          const std::string& actor_id  = {},
	          const std::string& payload   = {}) const;

	// ── Internal helpers ──────────────────────────────────────────────────────

	/** @brief Checks and resolves formation for all factions in `location_id`. */
	void check_formation(const LocationId& location_id);

	/** @brief Returns the active group count (non-removed). */
	int active_group_count() const;

	/** @brief Returns the active PG count (ACTIVE life state). */
	int active_pg_count() const;

	/** @brief Ends a hero's turn: resets sequence, emits EVT_PG_TURN_ENDED. */
	void end_hero_turn(const HeroId& hero_id);

	/**
	 * @brief Computes the target enemy faction for a hero's attack.
	 *
	 * Returns the first monster faction that has a valid target in
	 * the hero's current location.
	 */
	std::string enemy_faction_for_hero(const HeroId& hero_id) const;

	/**
	 * @brief Marks a monster instance as dead, updates the owning group,
	 *        and notifies the mission event system if the group is eliminated.
	 */
	void handle_monster_instance_death(const gmActor::ActorId& instance_id);

	/**
	 * @brief Builds and shuffles initial hand states from the PG roster.
	 *
	 * Called by `from_definition` after construction.  Shuffles each hero's
	 * mission_deck and deals `hand_limit` cards into the hand.
	 */
	void build_initial_hands(const std::vector<PgEntry>& roster);

	/**
	 * @brief Draws cards for a hero until the hand reaches hand_limit.
	 *
	 * If the draw pile is exhausted the discard pile is reshuffled back into
	 * the deck before drawing continues.  Emits EVT_DECK_RESHUFFLED when a
	 * reshuffle occurs.
	 */
	void draw_to_hand(const HeroId& hero_id);
};

} // namespace eldhom

#endif // ELDHOM_ENGINE_ELDHOMENGINE_HPP
