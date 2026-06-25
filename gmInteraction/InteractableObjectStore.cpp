/**
 * @file InteractableObjectStore.cpp
 * @brief Implementation of the interactable object registry and its JSON snapshot.
 */

#include "InteractableObjectStore.hpp"

#include "gmSave/gmSave.hpp"

#include <utility>

namespace gmInteraction
{

// to_json / from_json for the public type must live directly in the
// gmInteraction namespace so that nlohmann/json finds them through ADL.

/// @brief Serializes a single object record (state stored as canonical string).
void to_json(nlohmann::json& j, const InteractableObject& obj)
{
	j = nlohmann::json::object();
	j["id"] = obj.id;
	j["type"] = obj.type;
	j["state"] = interaction_state_to_string(obj.state);
	j["meta"] = obj.meta;
}

/// @brief Deserializes a single object record.
void from_json(const nlohmann::json& j, InteractableObject& obj)
{
	obj.id = j.at("id").get<InteractableObjectId>();
	obj.type = j.at("type").get<std::string>();
	obj.state = interaction_state_from_string(j.at("state").get<std::string>());
	obj.meta = j.value("meta", Metadata{});
}

namespace
{

/// @brief Schema version written by @ref InteractableObjectStore::export_snapshot_json.
constexpr uint32_t SNAPSHOT_VERSION = 1U;

/**
 * @brief Flat, serializable view of the whole store.
 */
struct InteractionSnapshot
{
	std::vector<InteractableObject> objects;
};

/// @brief Serializes the snapshot wrapper.
void to_json(nlohmann::json& j, const InteractionSnapshot& snap)
{
	j = nlohmann::json::object();
	j["objects"] = snap.objects;
}

/// @brief Deserializes the snapshot wrapper.
void from_json(const nlohmann::json& j, InteractionSnapshot& snap)
{
	snap.objects = j.value("objects", std::vector<InteractableObject>{});
}

} // namespace

InteractableObjectStore::InteractableObjectStore()
{
}

void InteractableObjectStore::create(InteractableObjectId id,
                                     const std::string&   type,
                                     InteractionState     state)
{
	if (_objects.find(id) != _objects.end())
	{
		throw EDuplicateObjectError("object id already exists: " + std::to_string(id));
	}

	InteractableObject obj;
	obj.id = id;
	obj.type = type;
	obj.state = state;
	_objects.emplace(id, std::move(obj));
}

void InteractableObjectStore::remove(InteractableObjectId id)
{
	if (_objects.erase(id) == 0)
	{
		throw EUnknownObjectError("unknown object id: " + std::to_string(id));
	}
}

bool InteractableObjectStore::has(InteractableObjectId id) const
{
	return _objects.find(id) != _objects.end();
}

const InteractableObject& InteractableObjectStore::get(InteractableObjectId id) const
{
	return _require(id);
}

void InteractableObjectStore::set_state(InteractableObjectId id, InteractionState state)
{
	_require(id).state = state;
}

InteractionState InteractableObjectStore::state_of(InteractableObjectId id) const
{
	return _require(id).state;
}

const std::string& InteractableObjectStore::type_of(InteractableObjectId id) const
{
	return _require(id).type;
}

void InteractableObjectStore::set_meta(InteractableObjectId id,
                                       const std::string&   key,
                                       const std::string&   value)
{
	_require(id).meta[key] = value;
}

const std::string& InteractableObjectStore::get_meta(InteractableObjectId id,
                                                     const std::string&   key) const
{
	const InteractableObject& obj = _require(id);
	const auto it = obj.meta.find(key);
	if (it == obj.meta.end())
	{
		throw EUnknownMetaKeyError("unknown meta key '" + key + "' on object "
		                           + std::to_string(id));
	}
	return it->second;
}

bool InteractableObjectStore::has_meta(InteractableObjectId id, const std::string& key) const
{
	const InteractableObject& obj = _require(id);
	return obj.meta.find(key) != obj.meta.end();
}

void InteractableObjectStore::remove_meta(InteractableObjectId id, const std::string& key)
{
	_require(id).meta.erase(key);
}

const Metadata& InteractableObjectStore::metadata(InteractableObjectId id) const
{
	return _require(id).meta;
}

std::vector<InteractableObjectId> InteractableObjectStore::all_ids() const
{
	std::vector<InteractableObjectId> out;
	out.reserve(_objects.size());
	for (const auto& kv : _objects)
	{
		out.push_back(kv.first);
	}
	return out;
}

std::size_t InteractableObjectStore::count() const
{
	return _objects.size();
}

void InteractableObjectStore::clear()
{
	_objects.clear();
}

void InteractableObjectStore::export_snapshot_json(const std::string& filepath) const
{
	InteractionSnapshot snap;
	snap.objects.reserve(_objects.size());
	for (const auto& kv : _objects)
	{
		snap.objects.push_back(kv.second);
	}
	gmSave::save_versioned(filepath, snap, SNAPSHOT_VERSION, 2);
}

void InteractableObjectStore::import_snapshot_json(const std::string& filepath)
{
	const std::optional<uint32_t> version = gmSave::peek_version(filepath);
	const uint32_t found_version = version.value_or(SNAPSHOT_VERSION);
	const InteractionSnapshot snap =
		gmSave::load_versioned<InteractionSnapshot>(filepath, found_version);

	_objects.clear();
	for (const InteractableObject& obj : snap.objects)
	{
		_objects.emplace(obj.id, obj);
	}
}

const InteractableObject& InteractableObjectStore::_require(InteractableObjectId id) const
{
	const auto it = _objects.find(id);
	if (it == _objects.end())
	{
		throw EUnknownObjectError("unknown object id: " + std::to_string(id));
	}
	return it->second;
}

InteractableObject& InteractableObjectStore::_require(InteractableObjectId id)
{
	const auto it = _objects.find(id);
	if (it == _objects.end())
	{
		throw EUnknownObjectError("unknown object id: " + std::to_string(id));
	}
	return it->second;
}

} // namespace gmInteraction
