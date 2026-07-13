/**
 * @file actors/ActorRoster.cpp
 * @brief ActorRoster implementation backed by gmActor::ActorStore.
 */

#include "actors/ActorRoster.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "gmActor/actors/HeroState.hpp"
#include "gmActor/actors/MonsterInstanceState.hpp"
#include "gmActor/core/Enums.hpp"

namespace {

/// @brief Derives a compact, game-unique display label from *actor_id*.
///
/// Rules:
///  - If the id ends with "_<digits>", base = first-letter-of-prefix (upper) + digits.
///    e.g. "hero_1" -> "H1", "monster_2" -> "M2"
///  - Otherwise base = first letter (upper).  e.g. "boss" -> "B"
///  - If the base is already taken, a numeric counter is appended until unique.
std::string make_label(
	const std::string& actor_id,
	std::unordered_set<std::string>& used)
{
	std::string prefix = actor_id;
	std::string num;

	auto pos = actor_id.rfind('_');
	if (pos != std::string::npos && pos + 1 < actor_id.size())
	{
		const std::string suffix = actor_id.substr(pos + 1);
		const bool is_digits = !suffix.empty() &&
			std::all_of(suffix.begin(), suffix.end(),
				[](unsigned char c) { return std::isdigit(c) != 0; });
		if (is_digits)
		{
			prefix = actor_id.substr(0, pos);
			num    = suffix;
		}
	}

	const char initial = prefix.empty()
		? '?'
		: static_cast<char>(std::toupper(static_cast<unsigned char>(prefix[0])));

	std::string base(1, initial);
	base += num;          // e.g. "H1" or "B"

	std::string label = base;
	int counter = 2;
	while (used.count(label))
	{
		label = base + std::to_string(counter++);
	}
	used.insert(label);
	return label;
}

gmActor::StatusInstance make_status(const std::string& status_id)
{
	gmActor::StatusInstance status;
	status.id = status_id;
	return status;
}

} // namespace

namespace gmDungeonBasic
{

ActorRoster::ActorRoster()
{
	// ToBeImplemented //
}

void ActorRoster::add_actor(const ActorInfo& info)
{
	if (has_actor(info.id))
	{
		throw std::invalid_argument("Actor already exists: " + info.id);
	}

	if (info.kind == DungeonActorKind::HERO)
	{
		gmActor::HeroState hero;
		hero.common.actor_id = info.id;
		hero.common.kind = gmActor::ActorKind::HERO;
		hero.common.display_name = info.id;
		hero.common.area_id = info.location;
		hero.common.max_hp = std::max(0, info.max_hp);
		hero.common.current_hp = std::clamp(info.hp, 0, hero.common.max_hp);
		hero.common.life_state = (hero.common.current_hp > 0)
			? gmActor::ActorLifeState::ACTIVE
			: gmActor::ActorLifeState::KO;
		hero.is_ko = (hero.common.life_state == gmActor::ActorLifeState::KO);
		hero.common.tags = info.tags;
		for (const std::string& status_id : info.statuses)
		{
			hero.common.statuses.push_back(make_status(status_id));
		}
		_store.add_hero(hero);
	}
	else
	{
		gmActor::MonsterInstanceState monster;
		monster.common.actor_id = info.id;
		monster.common.kind = gmActor::ActorKind::MONSTER_INSTANCE;
		monster.common.display_name = info.id;
		monster.common.area_id = info.location;
		monster.common.max_hp = std::max(0, info.max_hp);
		monster.common.current_hp = std::clamp(info.hp, 0, monster.common.max_hp);
		monster.common.life_state = (monster.common.current_hp > 0)
			? gmActor::ActorLifeState::ACTIVE
			: gmActor::ActorLifeState::KO;
		monster.common.tags = info.tags;
		monster.elite = (info.kind == DungeonActorKind::MONSTER_ELITE);
		monster.boss_part = (info.kind == DungeonActorKind::BOSS_MONSTER);
		for (const std::string& status_id : info.statuses)
		{
			monster.common.statuses.push_back(make_status(status_id));
		}
		_store.add_monster_instance(monster);
	}

	_kinds[info.id] = info.kind;
	_labels[info.id] = make_label(info.id, _used_labels);
	_attack[info.id] = std::max(0, info.attack);
	_defense[info.id] = std::max(0, info.defense);
	_insertion_order.push_back(info.id);
}

void ActorRoster::remove_actor(const std::string& actor_id)
{
	if (!has_actor(actor_id))
	{
		return;
	}

	std::vector<ActorInfo> survivors;
	survivors.reserve(_insertion_order.size());
	for (const std::string& id : _insertion_order)
	{
		if (id != actor_id)
		{
			survivors.push_back(snapshot_actor(id));
		}
	}

	_store = gmActor::ActorStore();
	_kinds.clear();
	_labels.clear();
	_used_labels.clear();
	_attack.clear();
	_defense.clear();
	_insertion_order.clear();

	for (const ActorInfo& info : survivors)
	{
		add_actor(info);
	}
}

bool ActorRoster::has_actor(const std::string& actor_id) const
{
	return _kinds.find(actor_id) != _kinds.end();
}

ActorInfo ActorRoster::get_actor(const std::string& actor_id) const
{
	if (!has_actor(actor_id))
	{
		throw std::invalid_argument("Unknown actor id: " + actor_id);
	}
	return snapshot_actor(actor_id);
}

std::vector<std::string> ActorRoster::all_actor_ids() const
{
	return _insertion_order;
}

std::vector<std::string> ActorRoster::heroes() const
{
	std::vector<std::string> out;
	for (const std::string& actor_id : _insertion_order)
	{
		if (_kinds.at(actor_id) == DungeonActorKind::HERO)
		{
			out.push_back(actor_id);
		}
	}
	return out;
}

std::vector<std::string> ActorRoster::enemies() const
{
	std::vector<std::string> out;
	for (const std::string& actor_id : _insertion_order)
	{
		const DungeonActorKind kind = _kinds.at(actor_id);
		if (kind == DungeonActorKind::MONSTER
			|| kind == DungeonActorKind::MONSTER_ELITE
			|| kind == DungeonActorKind::BOSS_MONSTER)
		{
			out.push_back(actor_id);
		}
	}
	return out;
}

std::vector<std::string> ActorRoster::actors_in_location(const std::string& location_id) const
{
	std::vector<std::string> out;
	for (const std::string& actor_id : _insertion_order)
	{
		if (common_ref(actor_id).area_id == location_id)
		{
			out.push_back(actor_id);
		}
	}
	return out;
}

void ActorRoster::set_hp(const std::string& actor_id, int hp)
{
	gmActor::ActorStateCommon& common = common_ref(actor_id);
	const int bounded_hp = std::clamp(hp, 0, std::max(0, common.max_hp));
	common.current_hp = bounded_hp;
	common.life_state = (bounded_hp > 0)
		? gmActor::ActorLifeState::ACTIVE
		: gmActor::ActorLifeState::KO;

	if (_kinds.at(actor_id) == DungeonActorKind::HERO)
	{
		_store.hero(actor_id).is_ko = (common.life_state == gmActor::ActorLifeState::KO);
	}
}

void ActorRoster::add_tag(const std::string& actor_id, const std::string& tag)
{
	gmActor::ActorStateCommon& common = common_ref(actor_id);
	const auto it = std::find(common.tags.begin(), common.tags.end(), tag);
	if (it == common.tags.end())
	{
		common.tags.push_back(tag);
	}
}

void ActorRoster::remove_tag(const std::string& actor_id, const std::string& tag)
{
	gmActor::ActorStateCommon& common = common_ref(actor_id);
	common.tags.erase(
		std::remove(common.tags.begin(), common.tags.end(), tag),
		common.tags.end());
}

bool ActorRoster::has_tag(const std::string& actor_id, const std::string& tag) const
{
	const gmActor::ActorStateCommon& common = common_ref(actor_id);
	return std::find(common.tags.begin(), common.tags.end(), tag) != common.tags.end();
}

void ActorRoster::add_status(const std::string& actor_id, const std::string& status_id)
{
	gmActor::ActorStateCommon& common = common_ref(actor_id);
	for (const gmActor::StatusInstance& status : common.statuses)
	{
		if (status.id == status_id)
		{
			return;
		}
	}
	common.statuses.push_back(make_status(status_id));
}

void ActorRoster::remove_status(const std::string& actor_id, const std::string& status_id)
{
	gmActor::ActorStateCommon& common = common_ref(actor_id);
	common.statuses.erase(
		std::remove_if(common.statuses.begin(),
			common.statuses.end(),
			[&status_id](const gmActor::StatusInstance& s)
			{
				return s.id == status_id;
			}),
		common.statuses.end());
}

bool ActorRoster::has_status(const std::string& actor_id, const std::string& status_id) const
{
	const gmActor::ActorStateCommon& common = common_ref(actor_id);
	for (const gmActor::StatusInstance& status : common.statuses)
	{
		if (status.id == status_id)
		{
			return true;
		}
	}
	return false;
}

void ActorRoster::move_to(const std::string& actor_id, const std::string& location_id)
{
	common_ref(actor_id).area_id = location_id;
}

void ActorRoster::reset()
{
	_store = gmActor::ActorStore();
	_insertion_order.clear();
	_kinds.clear();
	_labels.clear();
	_used_labels.clear();
	_attack.clear();
	_defense.clear();
}

ActorInfo ActorRoster::snapshot_actor(const std::string& actor_id) const
{
	ActorInfo info;
	info.id    = actor_id;
	info.kind  = _kinds.at(actor_id);
	info.label = _labels.count(actor_id) ? _labels.at(actor_id) : actor_id;
	info.attack  = _attack.count(actor_id) ? _attack.at(actor_id) : 0;
	info.defense = _defense.count(actor_id) ? _defense.at(actor_id) : 0;

	const gmActor::ActorStateCommon& common = common_ref(actor_id);
	info.hp = common.current_hp;
	info.max_hp = common.max_hp;
	info.location = common.area_id;
	info.tags = common.tags;
	for (const gmActor::StatusInstance& status : common.statuses)
	{
		info.statuses.push_back(status.id);
	}

	if (common.kind == gmActor::ActorKind::MONSTER_INSTANCE)
	{
		const gmActor::MonsterInstanceState& monster = _store.monster_instance(actor_id);
		if (monster.boss_part)
		{
			info.kind = DungeonActorKind::BOSS_MONSTER;
		}
		else if (monster.elite)
		{
			info.kind = DungeonActorKind::MONSTER_ELITE;
		}
		else
		{
			info.kind = DungeonActorKind::MONSTER;
		}
	}

	return info;
}

gmActor::ActorStateCommon& ActorRoster::common_ref(const std::string& actor_id)
{
	if (!has_actor(actor_id))
	{
		throw std::invalid_argument("Unknown actor id: " + actor_id);
	}

	const DungeonActorKind kind = _kinds.at(actor_id);
	if (kind == DungeonActorKind::HERO)
	{
		return _store.hero(actor_id).common;
	}

	return _store.monster_instance(actor_id).common;
}

const gmActor::ActorStateCommon& ActorRoster::common_ref(const std::string& actor_id) const
{
	if (!has_actor(actor_id))
	{
		throw std::invalid_argument("Unknown actor id: " + actor_id);
	}

	const DungeonActorKind kind = _kinds.at(actor_id);
	if (kind == DungeonActorKind::HERO)
	{
		return _store.hero(actor_id).common;
	}

	return _store.monster_instance(actor_id).common;
}

} // namespace gmDungeonBasic
