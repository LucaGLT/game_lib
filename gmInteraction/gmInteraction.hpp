#ifndef GMINTERACTION_GMINTERACTION_HPP
#define GMINTERACTION_GMINTERACTION_HPP

/**
 * @file gmInteraction.hpp
 * @brief Library entry header for gmInteraction.
 *
 * gmInteraction is a small, standalone C++17 library that manages a registry of
 * game-independent interactable objects (chests, levers, doors, item pickups,
 * …). It owns object data only; spatial placement lives in @c gmMap, linked via
 * @ref gmInteraction::MapInteractionBridge.
 *
 * Include this single header to access the whole public API.
 */

#include "GmInteractionError.hpp"
#include "InteractableObject.hpp"
#include "InteractableObjectStore.hpp"
#include "InteractionState.hpp"

namespace gmInteraction
{

/**
 * @brief Returns the gmInteraction library version string.
 *
 * @return Semantic version of the library.
 */
const char* version();

} // namespace gmInteraction

#endif // GMINTERACTION_GMINTERACTION_HPP
