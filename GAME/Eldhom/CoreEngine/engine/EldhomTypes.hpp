#ifndef ELDHOM_ENGINE_ELDHOMTYPES_HPP
#define ELDHOM_ENGINE_ELDHOMTYPES_HPP

/**
 * @file engine/EldhomTypes.hpp
 * @brief Shared types, enums and constants for Le Pergamene di Eldhôm engine.
 *
 * All game-specific identifiers, result codes, timeline constants and event
 * type string keys are defined here.  No gmXxx headers are included: this
 * file is the foundation layer that other engine headers build on.
 */

#include <cstdint>
#include <string>

namespace eldhom {

// ─────────────────────────────────────────────────────────────────────────────
// Type aliases
// ─────────────────────────────────────────────────────────────────────────────

using HeroId      = std::string; ///< Unique PG identifier
using GroupId     = std::string; ///< Unique Monster Group identifier
using InstanceId  = std::string; ///< Unique Monster Instance identifier
using LocationId  = std::string; ///< Unique location identifier
using CardId      = std::string; ///< Hero action card identifier
using BCardId     = std::string; ///< Monster behavior card identifier
using EventType   = std::string; ///< Engine event type string key
using EffectType  = std::string; ///< Card/step effect type string key

// ─────────────────────────────────────────────────────────────────────────────
// Simple action type (§3 — Azione Semplice del PG)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum SimpleActionType
 * @brief The four simple actions a PG may perform instead of playing a card.
 */
enum class SimpleActionType {
	MOVE,      ///< Movimento Semplice: sposta fino a 2 Loc, cost 1⌛
	ATTACK,    ///< Attacco Semplice: infliggi 1❌ su bersaglio vicino, cost 2⌛
	INTERACT,  ///< Interazione Semplice: usa elemento scena, cost 3⌛
	RECOVER    ///< Recupero Semplice: +1 PV, scarta/pesca una carta, cost 3⌛
};

// ─────────────────────────────────────────────────────────────────────────────
// Action result
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ActionResultCode
 * @brief Return code for engine action methods.
 */
enum class ActionResultCode {
	OK,
	ERR_NOT_YOUR_TURN,         ///< It is not this actor's activation turn
	ERR_CARD_NOT_IN_HAND,      ///< The card is not in the hero's hand
	ERR_CARD_NOT_PLAYABLE,     ///< SequenceEngine rejected the card type
	ERR_NO_SEQUENCE_ACTIVE,    ///< stop_sequence called with no active sequence
	ERR_UNKNOWN_ACTOR,         ///< actor_id not registered in the engine
	ERR_ACTOR_KO,              ///< Actor is KO and cannot act
	ERR_NO_VALID_TARGET        ///< Effect requires a target that cannot be found
};

/** @brief Result returned by engine action methods. */
struct ActionResult {
	ActionResultCode code    = ActionResultCode::OK;
	std::string      message;

	/** @brief Returns true if the action succeeded. */
	bool ok() const { return code == ActionResultCode::OK; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Timeline costs (⌛)
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Timeline cost for Azione Semplice: Movimento. */
constexpr int COST_SIMPLE_MOVE      = 1;
/** @brief Timeline cost for Azione Semplice: Attacco. */
constexpr int COST_SIMPLE_ATTACK    = 2;
/** @brief Timeline cost for Azione Semplice: Interazione. */
constexpr int COST_SIMPLE_INTERACT  = 3;
/** @brief Timeline cost for Azione Semplice: Recupero. */
constexpr int COST_SIMPLE_RECOVER   = 3;

/** @brief Timeline cost for basic monster move (§23). */
constexpr int COST_MONSTER_MOVE     = 1;
/** @brief Timeline cost for basic monster attack (§23). */
constexpr int COST_MONSTER_ATTACK   = 2;
/** @brief Timeline cost for basic monster wait (§23). */
constexpr int COST_MONSTER_WAIT     = 3;

// ─────────────────────────────────────────────────────────────────────────────
// Default PG stats
// ─────────────────────────────────────────────────────────────────────────────

constexpr int HP_BASE             = 6;   ///< Base HP for a hero
constexpr int HAND_LIMIT_BASE     = 5;   ///< Base hand card limit (5 + Livello)
constexpr int MEMORY_LIMIT_BASE   = 1;   ///< Base memory card limit
constexpr int MISSION_DECK_LIMIT  = 15;  ///< Base mission-deck card limit

// ─────────────────────────────────────────────────────────────────────────────
// Tie-break ranks (§2.2 — priorità attivazione)
// ─────────────────────────────────────────────────────────────────────────────

constexpr int RANK_HERO          = 1; ///< PG always resolves ties first
constexpr int RANK_ALLY_NPC      = 2;
constexpr int RANK_MONSTER_GROUP = 3;
constexpr int RANK_BOSS          = 4;

// ─────────────────────────────────────────────────────────────────────────────
// Defeat / victory thresholds (missione_01)
// ─────────────────────────────────────────────────────────────────────────────

constexpr int DEFEAT_TIME_LIMIT   = 60; ///< Sconfitta se mission_time >= 60⌛

// ─────────────────────────────────────────────────────────────────────────────
// Event type string constants
// ─────────────────────────────────────────────────────────────────────────────

// PG events
inline const EventType EVT_PG_PLAYED_CARD    = "eldhom.pg.played_card";
inline const EventType EVT_PG_SIMPLE_ACTION  = "eldhom.pg.simple_action";
inline const EventType EVT_PG_MOVED          = "eldhom.pg.moved";
inline const EventType EVT_PG_ATTACKED       = "eldhom.pg.attacked";
inline const EventType EVT_PG_HEALED         = "eldhom.pg.healed";
inline const EventType EVT_PG_KO             = "eldhom.pg.ko";
inline const EventType EVT_PG_TURN_STARTED   = "eldhom.pg.turn_started";
inline const EventType EVT_PG_TURN_ENDED     = "eldhom.pg.turn_ended";
inline const EventType EVT_SEQUENCE_STARTED  = "eldhom.pg.sequence_started";
inline const EventType EVT_SEQUENCE_ENDED    = "eldhom.pg.sequence_ended";
inline const EventType EVT_SEQUENCE_BROKEN   = "eldhom.pg.sequence_broken";

// Monster events (used as reaction triggers)
inline const EventType EVT_MONSTER_DAMAGED   = "eldhom.monster.damaged";
inline const EventType EVT_MONSTER_DEFEATED  = "eldhom.monster.defeated";
inline const EventType EVT_GROUP_ACTIVATED   = "eldhom.group.activated";
inline const EventType EVT_GROUP_ELIMINATED  = "eldhom.group.eliminated";

// Formation events
inline const EventType EVT_FORMATION_CHECKED = "eldhom.formation.checked";
inline const EventType EVT_FORMATION_CHANGED = "eldhom.formation.changed";

// Deck / hand events
inline const EventType EVT_HAND_CHANGED      = "eldhom.deck.hand_updated";
inline const EventType EVT_DECK_RESHUFFLED   = "eldhom.deck.reshuffled";

// Mission events
inline const EventType EVT_MISSION_TIME      = "eldhom.mission.time_advanced";
inline const EventType EVT_MISSION_VICTORY   = "eldhom.mission.victory";
inline const EventType EVT_MISSION_DEFEAT    = "eldhom.mission.defeat";

// Full-state snapshot (sent on mission start / GUI reconnect)
inline const EventType EVT_STATE_FULL        = "eldhom.state.full";

// Turn notification (sent after each turn ends, before the next begins)
inline const EventType EVT_TURN_NEXT_ACTOR   = "eldhom.turn.next_actor";

// Action result feedback
inline const EventType EVT_ACTION_RESULT     = "eldhom.action.result";

// ─────────────────────────────────────────────────────────────────────────────
// Network port constants (P7 — GUI bridge)
// ─────────────────────────────────────────────────────────────────────────────

namespace ports {
	/** @brief GUI TCP server port — engine connects to send events. */
	constexpr uint16_t EVENTS   = 9210;
	/** @brief Engine TCP server port — GUI connects to send commands. */
	constexpr uint16_t COMMANDS = 9211;
} // namespace ports

// ─────────────────────────────────────────────────────────────────────────────
// Command type string constants (GUI → engine)
// ─────────────────────────────────────────────────────────────────────────────

inline const std::string CMD_START_MISSION   = "eldhom.start_mission";
inline const std::string CMD_PLAY_CARD       = "eldhom.play_card";
inline const std::string CMD_SIMPLE_ACTION   = "eldhom.simple_action";
inline const std::string CMD_STOP_SEQUENCE   = "eldhom.stop_sequence";
inline const std::string CMD_REQUEST_STATE   = "eldhom.request_state";

} // namespace eldhom

#endif // ELDHOM_ENGINE_ELDHOMTYPES_HPP
