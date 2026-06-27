#ifndef ELDHOM_MISSION_MISSIONEVENTSYSTEM_HPP
#define ELDHOM_MISSION_MISSIONEVENTSYSTEM_HPP

/**
 * @file mission/MissionEventSystem.hpp
 * @brief Evaluates win/loss conditions and fires timeline milestone events.
 *
 * `MissionEventSystem` is a stateful service that:
 * 1. Tracks elapsed mission time (in ⌛ units)
 * 2. Fires registered `TimelineEvent` entries when their threshold is reached
 * 3. Evaluates victory and defeat conditions each time the mission time
 *    or actor state changes
 *
 * ### Usage
 *
 * The engine calls `advance_time(delta)` after each actor turn.
 * The system automatically fires events and updates `MissionOutcome`.
 * The engine polls `outcome()` to detect victory or defeat.
 *
 * ### Lifetime
 *
 * `MissionEventSystem` is constructed with a `MissionDefinition` and a
 * callback for outbound event emission.  It does not own actor state.
 */

#include "GAME/Eldhom/CoreEngine/mission/MissionDefinition.hpp"
#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"

#include <functional>
#include <string>
#include <vector>

namespace eldhom {

/**
 * @enum MissionOutcome
 * @brief High-level state of the current mission.
 */
enum class MissionOutcome {
	IN_PROGRESS, ///< Mission is ongoing
	VICTORY,     ///< Victory condition met
	DEFEAT       ///< Defeat condition met
};

/**
 * @brief Callback invoked when a timeline event fires.
 * @param event_type  String from `EldhomTypes.hpp` (e.g. EVT_MISSION_VICTORY).
 * @param payload     Event-specific payload (may be empty).
 */
using MissionEventCallback = std::function<void(const EventType&, const std::string&)>;

/**
 * @class MissionEventSystem
 * @brief Tracks mission time and evaluates win/loss conditions.
 */
class MissionEventSystem
{
public:
	/**
	 * @brief Constructs the system from a mission definition.
	 *
	 * @param def      Mission definition (conditions and timeline events).
	 * @param on_event Callback invoked when any mission event fires.
	 */
	MissionEventSystem(
		const MissionDefinition&  def,
		MissionEventCallback      on_event);

	// ── Time management ───────────────────────────────────────────────────────

	/**
	 * @brief Advances mission time by `delta` ⌛ units.
	 *
	 * Fires any `TimelineEvent` entries whose `at_time` falls within the
	 * new window.  Evaluates TIME_LIMIT defeat conditions.
	 *
	 * @param delta Non-negative time increment.
	 */
	void advance_time(int delta);

	/** @brief Returns current elapsed mission time (⌛). */
	int mission_time() const;

	// ── Win / loss evaluation ─────────────────────────────────────────────────

	/**
	 * @brief Evaluates victory condition `"ALL_MONSTERS_ELIMINATED"`.
	 *
	 * Call this when a monster group is eliminated to recheck.
	 *
	 * @param active_group_count Number of non-removed monster groups.
	 */
	void notify_group_eliminated(int active_group_count);

	/**
	 * @brief Evaluates defeat condition `"ALL_PG_KO"`.
	 *
	 * Call this when a PG goes KO to recheck.
	 *
	 * @param active_pg_count Number of PGs that are still ACTIVE (not KO/dead).
	 */
	void notify_pg_ko(int active_pg_count);

	/** @brief Returns the current mission outcome. */
	MissionOutcome outcome() const;

	/** @brief Returns true if the mission has ended (victory or defeat). */
	bool is_over() const;

private:
	MissionDefinition    _def;
	MissionEventCallback _on_event;

	int            _mission_time = 0;
	MissionOutcome _outcome      = MissionOutcome::IN_PROGRESS;

	// Track which non-repeating timeline events have already fired
	std::vector<bool> _fired_flags; ///< Parallel to _def.timeline_events

	void check_defeat_time_limit();
};

} // namespace eldhom

#endif // ELDHOM_MISSION_MISSIONEVENTSYSTEM_HPP
