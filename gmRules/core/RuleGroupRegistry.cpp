/**
 * @file core/RuleGroupRegistry.cpp
 * @brief Implementation of RuleGroupRegistry.
 */

#include "RuleGroupRegistry.hpp"

#include <algorithm>

namespace gmRules {

// ─────────────────────────────────────────────────────────────────────────────
// Registration
// ─────────────────────────────────────────────────────────────────────────────

void RuleGroupRegistry::register_group(const RuleGroup& group)
{
	if (group.group_id.empty())
	{
		throw ERuleGroupError("register_group: group_id must not be empty");
	}
	if (_registry.count(group.group_id) > 0)
	{
		throw ERuleGroupError(
			"register_group: group '" + group.group_id + "' is already registered");
	}
	_registry[group.group_id] = group;
	_registration_order.push_back(group.group_id);
}

void RuleGroupRegistry::unregister_group(const std::string& group_id)
{
	_active_groups.erase(group_id);
	_registry.erase(group_id);

	auto it = std::find(_registration_order.begin(), _registration_order.end(), group_id);
	if (it != _registration_order.end())
	{
		_registration_order.erase(it);
	}
}

bool RuleGroupRegistry::is_registered(const std::string& group_id) const
{
	return _registry.count(group_id) > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Activation / deactivation
// ─────────────────────────────────────────────────────────────────────────────

void RuleGroupRegistry::activate(const std::string& group_id)
{
	if (_registry.count(group_id) == 0)
	{
		throw ERuleGroupError(
			"activate: group '" + group_id + "' is not registered");
	}
	_active_groups.insert(group_id);
}

void RuleGroupRegistry::deactivate(const std::string& group_id)
{
	if (_registry.count(group_id) == 0)
	{
		throw ERuleGroupError(
			"deactivate: group '" + group_id + "' is not registered");
	}
	_active_groups.erase(group_id);
}

bool RuleGroupRegistry::is_active(const std::string& group_id) const
{
	return _active_groups.count(group_id) > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Query
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RuleId> RuleGroupRegistry::active_rule_ids() const
{
	std::vector<RuleId> result;
	for (const std::string& gid : _registration_order)
	{
		if (_active_groups.count(gid) == 0)
		{
			continue;
		}
		const std::vector<RuleId>& ids = _registry.at(gid).rule_ids;
		result.insert(result.end(), ids.begin(), ids.end());
	}
	return result;
}

const RuleGroup& RuleGroupRegistry::get_group(const std::string& group_id) const
{
	auto it = _registry.find(group_id);
	if (it == _registry.end())
	{
		throw ERuleGroupError(
			"get_group: group '" + group_id + "' is not registered");
	}
	return it->second;
}

bool RuleGroupRegistry::is_transient(const std::string& group_id) const
{
	auto it = _registry.find(group_id);
	if (it == _registry.end())
	{
		return false;
	}
	return it->second.lifecycle == RuleGroupLifecycle::TRANSIENT;
}

int RuleGroupRegistry::registered_count() const
{
	return static_cast<int>(_registry.size());
}

int RuleGroupRegistry::active_count() const
{
	return static_cast<int>(_active_groups.size());
}

} // namespace gmRules
