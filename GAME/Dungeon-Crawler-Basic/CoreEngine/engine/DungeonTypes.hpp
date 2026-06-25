#ifndef GMDUNGEONBASIC_DUNGEONTYPES_HPP
#define GMDUNGEONBASIC_DUNGEONTYPES_HPP

/**
 * @file engine/DungeonTypes.hpp
 * @brief Shared value types, enums and wire-protocol identifiers for the Dungeon Crawler engine.
 *
 * This header is the single source of truth for all symbolic names exchanged
 * between the CoreEngine subsystems and between CoreEngine and GUI. It has no
 * dependencies other than the C++17 standard library.
 */

#include <cstdint>
#include <string>

namespace gmDungeonBasic
{

// ── Game state enums ─────────────────────────────────────────────────────────

/**
 * @brief High-level phase of a dungeon session.
 */
enum class GamePhase
{
	BOOTSTRAP,     ///< Engine initialising, no session started yet.
	HERO_TURN,     ///< Waiting for the hero's action this turn.
	MONSTER_TURN,  ///< Monsters are acting (automatic or future expansion).
	GAME_OVER      ///< Match finished (hero defeated or dungeon cleared).
};

/**
 * @brief Classification of actors in the dungeon roster.
 *
 * Determines base statistics, AI behaviour and display in the GUI.
 */
enum class DungeonActorKind
{
	HERO,           ///< Player-controlled hero.
	MONSTER,        ///< Standard enemy unit.
	MONSTER_ELITE,  ///< Stronger variant of a standard monster.
	BOSS_MONSTER    ///< End-of-dungeon boss enemy.
};

/**
 * @brief Outcome of a finished dungeon session.
 */
enum class GameOutcome
{
	NONE,              ///< Session still in progress or not started.
	HERO_DEFEATED,     ///< All heroes have been reduced to 0 HP.
	DUNGEON_CLEARED    ///< The boss monster has been defeated.
};

// ── Network ports ─────────────────────────────────────────────────────────────

/**
 * @brief TCP ports used by the CoreEngine ↔ GUI bridge.
 *
 * Port 9200/9201 are used to avoid conflicts with the Tic-Tac-Toe game
 * (9100/9001) and with software that frequently occupies port 9000 on Windows.
 */
namespace ports
{
/// @brief Port where the GUI listens for engine events (Engine is TCP client).
constexpr uint16_t EVENTS   = 9200;
/// @brief Port where the Engine listens for GUI commands (Engine is TCP server).
constexpr uint16_t COMMANDS = 9201;
} // namespace ports

// ── Command identifiers (GUI → CoreEngine) ───────────────────────────────────

/**
 * @brief typeId strings for commands received from the GUI.
 *
 * Each command carries a JSON @c data object whose fields are defined in
 * @c info/wire-contract-v1.md.
 */
namespace command_id
{
/// @brief Start a new dungeon session. data: { map_file?: string, seed?: int }
constexpr const char* NEW_GAME  = "dungeon.new_game";
/// @brief Move the hero to an adjacent room. data: { hero_id: string, destination: string }
constexpr const char* MOVE      = "dungeon.move";
/// @brief Use a healing potion. data: { hero_id: string, target_id?: string }
constexpr const char* HEAL      = "dungeon.heal";
/// @brief Equip an available weapon. data: { hero_id: string, item_tag: string }
constexpr const char* EQUIP     = "dungeon.equip";
/// @brief End the current hero turn. data: { hero_id: string }
constexpr const char* END_TURN  = "dungeon.end_turn";
/// @brief View-only selection of a map area (no gameplay effect). data: { area_id: string }
constexpr const char* AREA_SELECTED     = "gmMap.ui.area_selected";
/// @brief Request the contents of a map area. data: { area_id: string, request_id?: string }
constexpr const char* AREA_INFO_REQUEST = "gmMap.area.info.request";
/// @brief Play a card from hand. data: { hero_id: string, card_id: string, target_id?: string }
constexpr const char* PLAY_CARD = "dungeon.play_card";
} // namespace command_id

// ── Event identifiers (CoreEngine → GUI) ─────────────────────────────────────

/**
 * @brief typeId strings for events emitted towards the GUI.
 *
 * Each event carries a JSON @c data object whose fields are defined in
 * @c info/wire-contract-v1.md.
 */
namespace event_id
{
/// @brief Full session start notification. data: { session_id: string, round: int }
constexpr const char* SESSION_STARTED   = "dungeon.session.started";
/// @brief Full snapshot of the dungeon map (rooms, connections, actor positions).
constexpr const char* MAP_SNAPSHOT      = "dungeon.map.snapshot";
/// @brief Full snapshot of all actors (id, kind, hp, max_hp, tags, location).
constexpr const char* ACTOR_SNAPSHOT    = "dungeon.actor.snapshot";
/// @brief Actor moved between rooms. data: { actor_id, from, to }
constexpr const char* ACTOR_MOVED       = "dungeon.actor.moved";
/// @brief Heal action executed. data: { actor_id, target_id, amount, hp_after }
constexpr const char* ACTOR_HEALED      = "dungeon.actor.healed";
/// @brief Weapon equipped. data: { actor_id, item_tag }
constexpr const char* ACTOR_EQUIPPED    = "dungeon.actor.equipped";
/// @brief Actor status added or removed. data: { actor_id, status_id, added: bool }
constexpr const char* STATUS_CHANGED    = "dungeon.actor.status_changed";
/// @brief Actor HP changed. data: { actor_id, delta, hp_after }
constexpr const char* HP_CHANGED        = "dungeon.actor.hp_changed";
/// @brief A new actor turn has started. data: { actor_id, round }
constexpr const char* TURN_STARTED      = "dungeon.turn.started";
/// @brief The current actor turn has ended. data: { actor_id }
constexpr const char* TURN_ENDED        = "dungeon.turn.ended";
/// @brief A requested action was rejected by the rule engine.
///        data: { reason: string, command: string }
constexpr const char* ACTION_REJECTED   = "dungeon.action.rejected";
/// @brief The dungeon session has ended. data: { outcome: string }
constexpr const char* GAME_OVER         = "dungeon.game.over";
/// @brief A card was played and its effects applied. data: { hero_id, card_id, effects_applied[] }
constexpr const char* CARD_PLAYED       = "dungeon.card.played";
/// @brief A card play was rejected. data: { hero_id, card_id, reason: string }
constexpr const char* CARD_REJECTED     = "dungeon.card.rejected";
/// @brief Contents of a selected map area (view-only response to AREA_INFO_REQUEST).
///        data: { area_id, actors: [...], interactables: [...], request_id? }
constexpr const char* AREA_INFO_RESPONSE = "gmMap.area.info.response";
} // namespace event_id

// ── Utility helpers ───────────────────────────────────────────────────────────

/**
 * @brief Converts a DungeonActorKind to its canonical string name.
 *
 * @param kind  Actor kind to convert.
 * @return      One of "HERO", "MONSTER", "MONSTER_ELITE", "BOSS_MONSTER".
 */
inline std::string actor_kind_to_string(DungeonActorKind kind)
{
	switch (kind)
	{
		case DungeonActorKind::HERO:           return "HERO";
		case DungeonActorKind::MONSTER:        return "MONSTER";
		case DungeonActorKind::MONSTER_ELITE:  return "MONSTER_ELITE";
		case DungeonActorKind::BOSS_MONSTER:   return "BOSS_MONSTER";
		default:                               return "UNKNOWN";
	}
}

/**
 * @brief Converts a GameOutcome to its canonical string name.
 *
 * @param outcome  Outcome value to convert.
 * @return         One of "NONE", "HERO_DEFEATED", "DUNGEON_CLEARED".
 */
inline std::string game_outcome_to_string(GameOutcome outcome)
{
	switch (outcome)
	{
		case GameOutcome::HERO_DEFEATED:    return "HERO_DEFEATED";
		case GameOutcome::DUNGEON_CLEARED:  return "DUNGEON_CLEARED";
		default:                            return "NONE";
	}
}

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_DUNGEONTYPES_HPP
