/**
 * @file stats/Health.cpp
 * @brief Implementation of Health helper functions.
 */

#include "gmActor/stats/Health.hpp"

#include <algorithm>

namespace gmActor {

bool has_health(const ActorStateCommon& actor)
{
    return actor.max_hp > 0;
}

int missing_hp(const ActorStateCommon& actor)
{
    return std::max(0, actor.max_hp - actor.current_hp);
}

bool is_alive(const ActorStateCommon& actor)
{
    return actor.life_state == ActorLifeState::ACTIVE;
}

bool is_ko(const ActorStateCommon& actor)
{
    return actor.life_state == ActorLifeState::KO;
}

void set_hp(ActorStateCommon& actor, int value)
{
    actor.current_hp = std::max(0, std::min(value, actor.max_hp));
}

void damage_hp(ActorStateCommon& actor, int amount)
{
    if (amount <= 0) return;
    actor.current_hp = std::max(0, actor.current_hp - amount);
}

void heal_hp(ActorStateCommon& actor, int amount)
{
    if (amount <= 0) return;
    actor.current_hp = std::min(actor.max_hp, actor.current_hp + amount);
}

} // namespace gmActor
