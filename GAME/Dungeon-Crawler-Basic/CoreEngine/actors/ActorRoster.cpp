/**
 * @file actors/ActorRoster.cpp
 * @brief Stub implementation of ActorRoster.
 *
 * Real integration with gmActor::ActorStore will be introduced in FASE B.
 */

#include "actors/ActorRoster.hpp"

namespace gmDungeonBasic
{

ActorRoster::ActorRoster()
{
	// ToBeImplemented //
}

void ActorRoster::add_actor(const ActorInfo& info)
{
	(void)info;
	// ToBeImplemented //
}

void ActorRoster::remove_actor(const std::string& actor_id)
{
	(void)actor_id;
	// ToBeImplemented //
}

bool ActorRoster::has_actor(const std::string& actor_id) const
{
	(void)actor_id;
	// ToBeImplemented //
	return false;
}

ActorInfo ActorRoster::get_actor(const std::string& actor_id) const
{
	(void)actor_id;
	// ToBeImplemented //
	return ActorInfo{};
}

std::vector<std::string> ActorRoster::all_actor_ids() const
{
	// ToBeImplemented //
	return {};
}

std::vector<std::string> ActorRoster::heroes() const
{
	// ToBeImplemented //
	return {};
}

std::vector<std::string> ActorRoster::enemies() const
{
	// ToBeImplemented //
	return {};
}

std::vector<std::string> ActorRoster::actors_in_location(const std::string& location_id) const
{
	(void)location_id;
	// ToBeImplemented //
	return {};
}

void ActorRoster::set_hp(const std::string& actor_id, int hp)
{
	(void)actor_id;
	(void)hp;
	// ToBeImplemented //
}

void ActorRoster::add_tag(const std::string& actor_id, const std::string& tag)
{
	(void)actor_id;
	(void)tag;
	// ToBeImplemented //
}

void ActorRoster::remove_tag(const std::string& actor_id, const std::string& tag)
{
	(void)actor_id;
	(void)tag;
	// ToBeImplemented //
}

bool ActorRoster::has_tag(const std::string& actor_id, const std::string& tag) const
{
	(void)actor_id;
	(void)tag;
	// ToBeImplemented //
	return false;
}

void ActorRoster::add_status(const std::string& actor_id, const std::string& status_id)
{
	(void)actor_id;
	(void)status_id;
	// ToBeImplemented //
}

void ActorRoster::remove_status(const std::string& actor_id, const std::string& status_id)
{
	(void)actor_id;
	(void)status_id;
	// ToBeImplemented //
}

bool ActorRoster::has_status(const std::string& actor_id, const std::string& status_id) const
{
	(void)actor_id;
	(void)status_id;
	// ToBeImplemented //
	return false;
}

void ActorRoster::move_to(const std::string& actor_id, const std::string& location_id)
{
	(void)actor_id;
	(void)location_id;
	// ToBeImplemented //
}

void ActorRoster::reset()
{
	// ToBeImplemented //
}

} // namespace gmDungeonBasic
