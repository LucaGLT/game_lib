#ifndef GMINTERACTION_MAPINTERACTIONBRIDGE_HPP
#define GMINTERACTION_MAPINTERACTIONBRIDGE_HPP

/**
 * @file bridges/MapInteractionBridge.hpp
 * @brief Cross-library adapter between gmInteraction and gmMap.
 *
 * This is the *only* place in gmInteraction allowed to include @c gmMap. It
 * keeps the two libraries decoupled: gmInteraction owns the object data, gmMap
 * owns the spatial placement (opaque @c InteractableObjectId per location), and
 * these free function templates keep the two views consistent.
 *
 * The templates are parameterised on the @c gmMap item type so they work with
 * any @c gmMap::gmMap<ItemT> instantiation.
 */

#include <string>
#include <vector>

#include "../InteractableObject.hpp"
#include "../InteractableObjectStore.hpp"
#include "../InteractionState.hpp"
#include "gmMap/gmMap.hpp"

namespace gmInteraction
{

/**
 * @brief Creates an object in the store and places it at a map location.
 *
 * @tparam ItemT     Item type of the target @c gmMap.
 * @param store      Object registry that will own the new object.
 * @param map        Spatial map that will record the placement.
 * @param location   Target location id in @p map.
 * @param id         Unique object id (shared by both libraries).
 * @param type       Domain type tag.
 * @param state      Initial state (default @ref InteractionState::IDLE).
 * @throws EDuplicateObjectError  If @p id already exists in @p store.
 */
template <typename ItemT>
void spawn_object(InteractableObjectStore&   store,
                  gmMap::gmMap<ItemT>&       map,
                  gmMap::LocationId          location,
                  InteractableObjectId       id,
                  const std::string&         type,
                  InteractionState           state = InteractionState::IDLE)
{
	store.create(id, type, state);
	map.place_interactable(location, id);
}

/**
 * @brief Removes an object from both the map location and the store.
 *
 * @tparam ItemT     Item type of the target @c gmMap.
 * @param store      Object registry to remove the object from.
 * @param map        Spatial map to detach the object from.
 * @param location   Location id the object is placed at.
 * @param id         Object id to remove.
 */
template <typename ItemT>
void despawn_object(InteractableObjectStore& store,
                    gmMap::gmMap<ItemT>&     map,
                    gmMap::LocationId        location,
                    InteractableObjectId     id)
{
	map.remove_interactable(location, id);
	if (store.has(id))
	{
		store.remove(id);
	}
}

/**
 * @brief Returns the full object records placed at a map location.
 *
 * Ids present on the map but absent from the store are skipped, so the result
 * is always consistent with the store.
 *
 * @tparam ItemT     Item type of the source @c gmMap.
 * @param store      Object registry to resolve ids against.
 * @param map        Spatial map to read placements from.
 * @param location   Location id to query.
 * @return           Vector of resolved @ref InteractableObject records.
 */
template <typename ItemT>
std::vector<InteractableObject> objects_at(const InteractableObjectStore& store,
                                           const gmMap::gmMap<ItemT>&     map,
                                           gmMap::LocationId              location)
{
	std::vector<InteractableObject> out;
	const std::vector<gmMap::InteractableObjectId> ids = map.interactables_at(location);
	out.reserve(ids.size());
	for (gmMap::InteractableObjectId id : ids)
	{
		if (store.has(id))
		{
			out.push_back(store.get(id));
		}
	}
	return out;
}

} // namespace gmInteraction

#endif // GMINTERACTION_MAPINTERACTIONBRIDGE_HPP
