/**
 * @file routers/PatternRouter.cpp
 * @brief Implementation of PatternRouter.
 */

#include "PatternRouter.hpp"

#include <algorithm>

namespace gmDispatch {

// ── Static helpers ────────────────────────────────────────────────────────────

bool PatternRouter::match_pattern(const std::string& pattern,
								  const std::string& typeId)
{
	// Broadcast wildcard
	if (pattern == "*") return true;

	// Exact match
	if (pattern == typeId) return true;

	// Prefix wildcard: "engine.*" matches "engine.tick", "engine.init", etc.
	// Pattern must end with ".*" and have at least one char before the dot.
	const std::size_t sz = pattern.size();
	if (sz >= 2 && pattern[sz - 1] == '*' && pattern[sz - 2] == '.') {
		// prefix = "engine." (everything up to and including the dot)
		const std::string prefix = pattern.substr(0, sz - 1);
		return typeId.size() >= prefix.size() &&
			   typeId.compare(0, prefix.size(), prefix) == 0;
	}

	return false;
}

bool PatternRouter::is_targeted(const std::shared_ptr<IChannel>& channel,
								const Envelope&                  envelope)
{
	// Broadcast: no targets specified
	if (envelope.targets.empty()) return true;

	// Anonymous channel: always receives
	const std::string chName = channel->name();
	if (chName.empty()) return true;

	// Named channel: receives only if its name is in the targets list
	for (const std::string& t : envelope.targets) {
		if (t == chName) return true;
	}
	return false;
}

// ── IRouter interface ─────────────────────────────────────────────────────────

void PatternRouter::subscribe(const std::string&        pattern,
							   std::shared_ptr<IChannel> channel)
{
	_routes[pattern].push_back(std::move(channel));
}

void PatternRouter::unsubscribe(const std::string&        pattern,
								 std::shared_ptr<IChannel> channel)
{
	std::map<std::string,
			 std::vector<std::shared_ptr<IChannel>>>::iterator it =
		_routes.find(pattern);
	if (it == _routes.end()) return;

	std::vector<std::shared_ptr<IChannel>>& vec = it->second;
	vec.erase(std::remove(vec.begin(), vec.end(), channel), vec.end());
	if (vec.empty()) _routes.erase(it);
}

void PatternRouter::route(const Envelope& envelope)
{
	for (std::pair<const std::string,
				   std::vector<std::shared_ptr<IChannel>>>& kv : _routes)
	{
		if (!match_pattern(kv.first, envelope.typeId)) continue;

		for (std::shared_ptr<IChannel>& ch : kv.second)
		{
			if (is_targeted(ch, envelope))
			{
				ch->send(envelope);
			}
		}
	}
}

void PatternRouter::flush()
{
	std::unordered_set<IChannel*> seen;
	for (std::pair<const std::string,
				   std::vector<std::shared_ptr<IChannel>>>& kv : _routes)
	{
		for (std::shared_ptr<IChannel>& ch : kv.second)
		{
			if (seen.insert(ch.get()).second)
			{
				ch->flush();
			}
		}
	}
}

} // namespace gmDispatch
