/**
 * @file flow/PhaseContext.cpp
 * @brief Implementation of gmFlow::PhaseContext.
 */

#include "gmFlow/flow/PhaseContext.hpp"

#include <stdexcept>

namespace gmFlow {

PhaseContext::PhaseContext(GameContext& parent, std::string scope_prefix)
	: GameContext(parent.session_id(),
	              parent.state(),
	              parent.actor_registry(),
	              parent.event_bus())
	, _scope_prefix(std::move(scope_prefix))
{
	if (_scope_prefix.empty())
	{
		throw std::invalid_argument(
			"PhaseContext: scope_prefix must not be empty");
	}
}

const std::string& PhaseContext::scope_prefix() const
{
	return _scope_prefix;
}

} // namespace gmFlow
