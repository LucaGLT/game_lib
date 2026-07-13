#ifndef GMINTERACTION_INTERACTABLEOBJECTSTORE_HPP
#define GMINTERACTION_INTERACTABLEOBJECTSTORE_HPP

/**
 * @file InteractableObjectStore.hpp
 * @brief Authoritative registry of interactable objects.
 *
 * The store owns the object *data* (type, state, metadata). It is purely a
 * registry: it knows nothing about where objects are placed. Spatial placement
 * is tracked by @c gmMap via opaque ids; the two are linked through
 * @ref gmInteraction::MapInteractionBridge.
 *
 * @note Not thread-safe.
 */

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "GmInteractionError.hpp"
#include "InteractableObject.hpp"
#include "InteractionState.hpp"

namespace gmInteraction
{

/**
 * @brief In-memory registry of @ref InteractableObject records.
 */
class InteractableObjectStore
{
public:
	/// @brief Constructs an empty store.
	InteractableObjectStore();

	/**
	 * @brief Creates a new interactable object.
	 *
	 * @param id     Unique object id.
	 * @param type   Domain type tag.
	 * @param state  Initial state (default @ref InteractionState::IDLE).
	 * @throws EDuplicateObjectError  If @p id already exists.
	 */
	void create(InteractableObjectId id,
	            const std::string&   type,
	            InteractionState     state = InteractionState::IDLE);

	/**
	 * @brief Removes an object from the store.
	 *
	 * @param id  Id of the object to remove.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	void remove(InteractableObjectId id);

	/**
	 * @brief Checks whether an object exists.
	 *
	 * @param id  Id to query.
	 * @return    @c true if the object is in the store.
	 */
	bool has(InteractableObjectId id) const;

	/**
	 * @brief Returns the full record for an object.
	 *
	 * @param id  Id of the object.
	 * @return    Const reference to the stored record.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	const InteractableObject& get(InteractableObjectId id) const;

	/**
	 * @brief Sets the lifecycle state of an object.
	 *
	 * @param id     Target object id.
	 * @param state  New state value.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	void set_state(InteractableObjectId id, InteractionState state);

	/**
	 * @brief Returns the current state of an object.
	 *
	 * @param id  Object id to query.
	 * @return    The object's state.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	InteractionState state_of(InteractableObjectId id) const;

	/**
	 * @brief Returns the type tag of an object.
	 *
	 * @param id  Object id to query.
	 * @return    Const reference to the type string.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	const std::string& type_of(InteractableObjectId id) const;

	/**
	 * @brief Sets (or overwrites) a metadata entry.
	 *
	 * @param id     Target object id.
	 * @param key    Metadata key.
	 * @param value  Metadata value.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	void set_meta(InteractableObjectId id,
	              const std::string&   key,
	              const std::string&   value);

	/**
	 * @brief Reads a metadata value.
	 *
	 * @param id   Target object id.
	 * @param key  Metadata key.
	 * @return     Const reference to the stored value.
	 * @throws EUnknownObjectError   If @p id does not exist.
	 * @throws EUnknownMetaKeyError  If @p key is not present.
	 */
	const std::string& get_meta(InteractableObjectId id, const std::string& key) const;

	/**
	 * @brief Checks whether a metadata key exists.
	 *
	 * @param id   Target object id.
	 * @param key  Metadata key.
	 * @return     @c true if the key is present.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	bool has_meta(InteractableObjectId id, const std::string& key) const;

	/**
	 * @brief Removes a metadata key (no-op if absent).
	 *
	 * @param id   Target object id.
	 * @param key  Metadata key to remove.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	void remove_meta(InteractableObjectId id, const std::string& key);

	/**
	 * @brief Returns the full metadata map of an object.
	 *
	 * @param id  Target object id.
	 * @return    Const reference to the metadata map.
	 * @throws EUnknownObjectError  If @p id does not exist.
	 */
	const Metadata& metadata(InteractableObjectId id) const;

	/**
	 * @brief Returns all object ids in the store.
	 *
	 * @return  Vector of ids in unspecified order.
	 */
	std::vector<InteractableObjectId> all_ids() const;

	/**
	 * @brief Returns the number of objects in the store.
	 *
	 * @return  Object count.
	 */
	std::size_t count() const;

	/// @brief Removes all objects from the store.
	void clear();

	/**
	 * @brief Writes the whole store to a versioned JSON file (schema v1).
	 *
	 * @param filepath  Destination file path.
	 * @throws gmSave::EFileWriteError  On I/O failure.
	 */
	void export_snapshot_json(const std::string& filepath) const;

	/**
	 * @brief Replaces the store contents from a versioned JSON file.
	 *
	 * @param filepath  Source file path.
	 * @throws gmSave::EFileReadError   If the file cannot be read.
	 * @throws gmSave::EJsonParseError  If the file is malformed.
	 */
	void import_snapshot_json(const std::string& filepath);

private:
	const InteractableObject& _require(InteractableObjectId id) const;
	InteractableObject&       _require(InteractableObjectId id);

	std::unordered_map<InteractableObjectId, InteractableObject> _objects;
};

} // namespace gmInteraction

#endif // GMINTERACTION_INTERACTABLEOBJECTSTORE_HPP
