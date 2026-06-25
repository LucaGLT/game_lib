/**
 * @file players/Players.cpp
 * @brief Implementation of player identity and status tracking over gmActor.
 */

#include "Players.hpp"

#include "gmActor/actors/HeroState.hpp"
#include "gmActor/core/Enums.hpp"
#include "gmActor/statuses/StatusInstance.hpp"

#include <algorithm>

namespace gmTris
{

const std::string Players::STATUS_ACTIVE_TURN = "ACTIVE_TURN";
const std::string Players::STATUS_WINNER      = "WINNER";
const std::string Players::STATUS_DRAW        = "DRAW";
const std::string Players::SOURCE_ENGINE      = "gmTris.engine";

Players::Players()
{
	for (Mark mark : {Mark::X, Mark::O})
	{
		gmActor::HeroState hero;
		hero.common.actor_id     = id_of(mark);
		hero.common.kind         = gmActor::ActorKind::HERO;
		hero.common.display_name = (mark == Mark::O) ? "Player O" : "Player X";
		hero.common.faction_id   = (mark == Mark::O) ? "team_o" : "team_x";
		_store.add_hero(std::move(hero));
	}
}

std::string Players::id_of(Mark mark)
{
	return (mark == Mark::O) ? "Player_O" : "Player_X";
}

std::string Players::actor_id(Mark mark) const
{
	return _store.common(id_of(mark)).actor_id;
}

std::string Players::display_name(Mark mark) const
{
	return _store.common(id_of(mark)).display_name;
}

void Players::add_status(Mark mark, const std::string &status)
{
	std::vector<gmActor::StatusInstance> &statuses = _store.common(id_of(mark)).statuses;
	const auto it = std::find_if(statuses.begin(), statuses.end(),
		[&status](const gmActor::StatusInstance &entry) { return entry.id == status; });
	if (it != statuses.end())
	{
		return;
	}
	gmActor::StatusInstance instance;
	instance.id        = status;
	instance.source_id = SOURCE_ENGINE;
	instance.stacks    = 1;
	statuses.push_back(std::move(instance));
}

void Players::remove_status(Mark mark, const std::string &status)
{
	std::vector<gmActor::StatusInstance> &statuses = _store.common(id_of(mark)).statuses;
	statuses.erase(std::remove_if(statuses.begin(), statuses.end(),
		[&status](const gmActor::StatusInstance &entry) { return entry.id == status; }),
		statuses.end());
}

bool Players::has_status(Mark mark, const std::string &status) const
{
	const std::vector<gmActor::StatusInstance> &statuses = _store.common(id_of(mark)).statuses;
	return std::any_of(statuses.begin(), statuses.end(),
		[&status](const gmActor::StatusInstance &entry) { return entry.id == status; });
}

void Players::set_active(Mark mark)
{
	_active = mark;
	remove_status(opponent(mark), STATUS_ACTIVE_TURN);
	add_status(mark, STATUS_ACTIVE_TURN);
}

Mark Players::active() const
{
	return _active;
}

void Players::mark_winner(Mark mark)
{
	remove_status(Mark::X, STATUS_ACTIVE_TURN);
	remove_status(Mark::O, STATUS_ACTIVE_TURN);
	add_status(mark, STATUS_WINNER);
}

void Players::mark_draw()
{
	remove_status(Mark::X, STATUS_ACTIVE_TURN);
	remove_status(Mark::O, STATUS_ACTIVE_TURN);
	add_status(Mark::X, STATUS_DRAW);
	add_status(Mark::O, STATUS_DRAW);
}

void Players::reset_statuses()
{
	for (Mark mark : {Mark::X, Mark::O})
	{
		_store.common(id_of(mark)).statuses.clear();
	}
}

Mark Players::opponent(Mark mark)
{
	return (mark == Mark::X) ? Mark::O : Mark::X;
}

const gmActor::ActorStore &Players::store() const
{
	return _store;
}

} // namespace gmTris

