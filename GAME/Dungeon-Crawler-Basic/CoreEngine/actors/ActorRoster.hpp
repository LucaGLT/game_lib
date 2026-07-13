#ifndef GMDUNGEONBASIC_ACTORROSTER_HPP
#define GMDUNGEONBASIC_ACTORROSTER_HPP

/**
 * @file actors/ActorRoster.hpp
 * @brief Dungeon actor registry: hero, monsters, elites and boss management.
 *
 * ActorRoster wraps @c gmActor::ActorStore to maintain the complete set of
 * actors in a dungeon session. It exposes a domain-level API using string actor
 * identifiers and @ref DungeonActorKind values, hiding the internal gmActor
 * type details from the rest of the engine.
 *
 * Supported actor categories:
 * - @c HERO: player-controlled, one per session in v1.
 * - @c MONSTER: standard enemy.
 * - @c MONSTER_ELITE: stronger variant.
 * - @c BOSS_MONSTER: session end-boss.
 *
 * @note Not thread-safe.
 */

#include "engine/DungeonTypes.hpp"
#include "gmActor/actors/ActorStore.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gmDungeonBasic
{

/**
 * @brief Snapshot data for a single actor.
 *
 * Used for snapshot events and rule evaluations.
 */
struct ActorInfo
{
	std::string              id;        ///< Unique actor identifier.
	std::string              label;     ///< Short game-unique display label (e.g. "H1", "M2").
	DungeonActorKind         kind;      ///< Actor classification.
	int                      hp;        ///< Current hit points.
	int                      max_hp;    ///< Maximum hit points.
	int                      attack = 0;  ///< Base attack value (damage stat).
	int                      defense = 0; ///< Passive defense value (damage reduction).
	std::string              location;  ///< Current room identifier.
	std::vector<std::string> tags;      ///< Active tags (equipped_weapon, has_potion, …).
	std::vector<std::string> statuses;  ///< Active status ids (defended, poisoned, …).
};

/**
 * @brief Domain-level registry of all actors in a dungeon session.
 *
 * Internally backed by gmActor. All methods work with string actor ids so
 * callers never handle gmActor internal types directly.
 */
class ActorRoster
{
public:
	/// @brief Constructs an empty roster.
	ActorRoster();

	/**
	 * @brief Registers a new actor in the roster.
	 *
	 * @param info  Actor data including id, kind, HP, location and initial tags.
	 * @throws std::invalid_argument  If an actor with the same id already exists.
	 */
	void add_actor(const ActorInfo& info);

	/**
	 * @brief Removes an actor from the roster.
	 *
	 * @param actor_id  Id of the actor to remove.
	 */
	void remove_actor(const std::string& actor_id);

	/**
	 * @brief Checks whether an actor exists.
	 *
	 * @param actor_id  Id to query.
	 * @return          @c true if the actor is in the roster.
	 */
	bool has_actor(const std::string& actor_id) const;

	/**
	 * @brief Returns a snapshot of one actor's current state.
	 *
	 * @param actor_id  Id of the actor to retrieve.
	 * @return          @ref ActorInfo snapshot.
	 * @throws std::invalid_argument  If the actor does not exist.
	 */
	ActorInfo get_actor(const std::string& actor_id) const;

	/**
	 * @brief Returns all actor identifiers currently in the roster.
	 *
	 * @return  Vector of id strings in unspecified order.
	 */
	std::vector<std::string> all_actor_ids() const;

	/**
	 * @brief Returns ids of all actors of kind HERO.
	 *
	 * @return  Vector of hero id strings.
	 */
	std::vector<std::string> heroes() const;

	/**
	 * @brief Returns ids of all enemy actors (MONSTER, MONSTER_ELITE, BOSS_MONSTER).
	 *
	 * @return  Vector of enemy id strings.
	 */
	std::vector<std::string> enemies() const;

	/**
	 * @brief Returns ids of all actors currently in a specific room.
	 *
	 * @param location_id  Room identifier to query.
	 * @return             Vector of actor id strings.
	 */
	std::vector<std::string> actors_in_location(const std::string& location_id) const;

	/**
	 * @brief Updates the current HP of an actor.
	 *
	 * @param actor_id  Target actor.
	 * @param hp        New HP value (clamped to [0, max_hp] in FASE B).
	 */
	void set_hp(const std::string& actor_id, int hp);

	/**
	 * @brief Adds a tag to an actor.
	 *
	 * @param actor_id  Target actor.
	 * @param tag       Tag string to add (e.g. "equipped_weapon", "has_potion").
	 */
	void add_tag(const std::string& actor_id, const std::string& tag);

	/**
	 * @brief Removes a tag from an actor.
	 *
	 * @param actor_id  Target actor.
	 * @param tag       Tag string to remove.
	 */
	void remove_tag(const std::string& actor_id, const std::string& tag);

	/**
	 * @brief Checks whether an actor has a specific tag.
	 *
	 * @param actor_id  Actor to query.
	 * @param tag       Tag string to look for.
	 * @return          @c true if the tag is present.
	 */
	bool has_tag(const std::string& actor_id, const std::string& tag) const;

	/**
	 * @brief Applies a status to an actor.
	 *
	 * @param actor_id   Target actor.
	 * @param status_id  Status identifier (e.g. "defended", "poisoned", "stunned").
	 */
	void add_status(const std::string& actor_id, const std::string& status_id);

	/**
	 * @brief Removes a status from an actor.
	 *
	 * @param actor_id   Target actor.
	 * @param status_id  Status identifier to remove.
	 */
	void remove_status(const std::string& actor_id, const std::string& status_id);

	/**
	 * @brief Checks whether an actor has a specific status.
	 *
	 * @param actor_id   Actor to query.
	 * @param status_id  Status identifier to look for.
	 * @return           @c true if the status is active.
	 */
	bool has_status(const std::string& actor_id, const std::string& status_id) const;

	/**
	 * @brief Moves an actor to a new room.
	 *
	 * @param actor_id     Actor to move.
	 * @param location_id  Destination room identifier.
	 */
	void move_to(const std::string& actor_id, const std::string& location_id);

	/// @brief Removes all actors. Resets to empty state.
	void reset();

private:
	ActorInfo snapshot_actor(const std::string& actor_id) const;
	gmActor::ActorStateCommon& common_ref(const std::string& actor_id);
	const gmActor::ActorStateCommon& common_ref(const std::string& actor_id) const;

	gmActor::ActorStore _store;
	std::vector<std::string> _insertion_order;
	std::unordered_map<std::string, DungeonActorKind> _kinds;
	std::unordered_map<std::string, std::string> _labels;      ///< actor_id → display label.
	std::unordered_set<std::string> _used_labels;              ///< Labels already assigned.
	std::unordered_map<std::string, int> _attack;              ///< actor_id → base attack value.
	std::unordered_map<std::string, int> _defense;             ///< actor_id → passive defense value.
};

} // namespace gmDungeonBasic

#endif // GMDUNGEONBASIC_ACTORROSTER_HPP
