/**
 * @file adapters/GmFlowActorAdapter.cpp
 * @brief Stub implementation of GmFlowActorAdapter free functions.
 */

#include "gmActor/adapters/GmFlowActorAdapter.hpp"

namespace gmActor {

void populate_flow_registry(const ActorStore& store,
                             gmFlow::ActorRegistry& registry)
{
    // TODO Phase 4: iterate timeline_actor_ids(), build gmFlow::Actor for each,
    // add to registry.
    (void)store;
    (void)registry;
}

} // namespace gmActor
