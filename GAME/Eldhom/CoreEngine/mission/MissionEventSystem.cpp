/**
 * @file mission/MissionEventSystem.cpp
 * @brief Implementation of MissionEventSystem.
 */

#include "GAME/Eldhom/CoreEngine/mission/MissionEventSystem.hpp"

namespace eldhom {

MissionEventSystem::MissionEventSystem(
	const MissionDefinition& def,
	MissionEventCallback      on_event)
	: _def(def)
	, _on_event(std::move(on_event))
	, _fired_flags(def.timeline_events.size(), false)
{
}

void MissionEventSystem::advance_time(int delta)
{
	if (_outcome != MissionOutcome::IN_PROGRESS) { return; }
	if (delta <= 0) { return; }

	int prev_time   = _mission_time;
	_mission_time  += delta;

	// Fire timeline events in definition order
	for (std::size_t i = 0; i < _def.timeline_events.size(); ++i)
	{
		const TimelineEvent& ev = _def.timeline_events[i];

		if (ev.repeating)
		{
			// Fire every `at_time` ticks: check if a new multiple was crossed
			if (ev.at_time > 0)
			{
				int prev_multiple = prev_time / ev.at_time;
				int curr_multiple = _mission_time / ev.at_time;
				if (curr_multiple > prev_multiple)
				{
					_on_event(ev.effect_type, ev.payload);
				}
			}
		}
		else
		{
			if (!_fired_flags[i] &&
			    prev_time < ev.at_time &&
			    _mission_time >= ev.at_time)
			{
				_fired_flags[i] = true;
				_on_event(ev.effect_type, ev.payload);
			}
		}
	}

	// Check time-based defeat conditions
	check_defeat_time_limit();

	// Emit generic time-advanced event
	_on_event(EVT_MISSION_TIME, std::to_string(_mission_time));
}

int MissionEventSystem::mission_time() const
{
	return _mission_time;
}

void MissionEventSystem::notify_group_eliminated(int active_group_count)
{
	if (_outcome != MissionOutcome::IN_PROGRESS) { return; }

	for (const VictoryCondition& vc : _def.victory_conditions)
	{
		if (vc.type == "ALL_MONSTERS_ELIMINATED" && active_group_count == 0)
		{
			_outcome = MissionOutcome::VICTORY;
			_on_event(EVT_MISSION_VICTORY, "ALL_MONSTERS_ELIMINATED");
			return;
		}
	}
}

void MissionEventSystem::notify_pg_ko(int active_pg_count)
{
	if (_outcome != MissionOutcome::IN_PROGRESS) { return; }

	for (const DefeatCondition& dc : _def.defeat_conditions)
	{
		if (dc.type == "ALL_PG_KO" && active_pg_count == 0)
		{
			_outcome = MissionOutcome::DEFEAT;
			_on_event(EVT_MISSION_DEFEAT, "ALL_PG_KO");
			return;
		}
	}
}

void MissionEventSystem::notify_pg_moved(
	const HeroId&      hero_id,
	const LocationId&  new_loc,
	const std::string& item_carried)
{
	if (_outcome != MissionOutcome::IN_PROGRESS) { return; }

	for (const VictoryCondition& vc : _def.victory_conditions)
	{
		if (vc.type != "PG_REACHED_EXIT") { continue; }
		if (new_loc != vc.target_location) { continue; }
		if (!vc.require_item.empty() && item_carried != vc.require_item) { continue; }

		_outcome = MissionOutcome::VICTORY;
		_on_event(EVT_MISSION_VICTORY, hero_id + " reached " + new_loc);
		return;
	}
}

MissionOutcome MissionEventSystem::outcome() const
{
	return _outcome;
}

bool MissionEventSystem::is_over() const
{
	return _outcome != MissionOutcome::IN_PROGRESS;
}

// ── Private ───────────────────────────────────────────────────────────────────

void MissionEventSystem::check_defeat_time_limit()
{
	if (_outcome != MissionOutcome::IN_PROGRESS) { return; }

	for (const DefeatCondition& dc : _def.defeat_conditions)
	{
		if (dc.type == "TIME_LIMIT" && _mission_time >= dc.threshold)
		{
			_outcome = MissionOutcome::DEFEAT;
			_on_event(EVT_MISSION_DEFEAT,
				"TIME_LIMIT:" + std::to_string(_mission_time));
			return;
		}
	}
}

} // namespace eldhom
