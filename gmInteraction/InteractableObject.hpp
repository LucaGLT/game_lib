#ifndef GMINTERACTION_INTERACTABLEOBJECT_HPP
#define GMINTERACTION_INTERACTABLEOBJECT_HPP

/**
 * @file InteractableObject.hpp
 * @brief Minimal, game-independent interactable object record.
 *
 * An @ref InteractableObject is pure data: an opaque id, a free-form @c type
 * string, a coarse @ref InteractionState and a string key/value metadata bag.
 * Spatial placement is *not* stored here — that responsibility belongs to
 * @c gmMap, which keeps the @ref InteractableObjectId per location.
 */

#include <cstdint>
#include <string>
#include <unordered_map>

#include "InteractionState.hpp"

namespace gmInteraction
{

/**
 * @brief Opaque, stable identifier for an interactable object.
 *
 * Matches @c gmMap::InteractableObjectId (both are @c uint64_t) so the same id
 * can be used in both libraries without conversion.
 */
using InteractableObjectId = uint64_t;

/**
 * @brief Free-form, serializable string metadata bag.
 */
using Metadata = std::unordered_map<std::string, std::string>;

/**
 * @brief Data record describing a single interactable object.
 */
struct InteractableObject
{
	InteractableObjectId id = 0;                          ///< Opaque unique id.
	std::string          type;                            ///< Domain type tag (e.g. "chest").
	InteractionState     state = InteractionState::IDLE;  ///< Current lifecycle state.
	Metadata             meta;                            ///< Optional key/value metadata.
};

} // namespace gmInteraction

#endif // GMINTERACTION_INTERACTABLEOBJECT_HPP
