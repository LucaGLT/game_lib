/**
 * @file adapters/GmFlowActorAdapter.cpp
 * @brief Stub implementation of GmFlowActorAdapter free functions.
 */

#include "gmActor/adapters/GmFlowActorAdapter.hpp"

namespace gmActor {

void populate_flow_registry(const ActorStore& store,
                             gmFlow::ActorRegistry& registry)
{
	for (const ActorId& id : store.timeline_actor_ids())
	{
		ActorKind k = store.actor_kind(id);

		if (k == ActorKind::MONSTER_GROUP)
		{
			registry.add(make_flow_actor_from_group(store.monster_group(id)));
		}
		else if (k == ActorKind::MISSION_SYSTEM)
		{
			const MissionSystemState& sys = store.mission_system();
			gmFlow::Actor actor(sys.actor_id, gmFlow::ActorType::SYSTEM);
			actor.set_display_name(sys.display_name);
			registry.add(std::move(actor));
		}
		else
		{
			registry.add(make_flow_actor(store, id));
		}
	}
}

} // namespace gmActor
