/**
 * @file bridges/ActionGateway.cpp
 * @brief Implementation of gmFlow::ActionGateway.
 */

#include "gmFlow/bridges/ActionGateway.hpp"
#include "gmFlow/events/EventType.hpp"

#include <stdexcept>

namespace gmFlow {

ActionGateway::ActionGateway(std::unique_ptr<IAction> inner,
                             FlowRulesPayload         payload,
                             ActionPreCheck           pre_check,
                             ActionPostHook           post_hook)
	: _inner(std::move(inner))
	, _payload(std::move(payload))
	, _pre_check(std::move(pre_check))
	, _post_hook(std::move(post_hook))
{
	if (!_inner)
	{
		throw std::invalid_argument(
			"ActionGateway: inner action must not be null");
	}
}

// ─────────────────────────────────────────────────────────────────────────────

ValidationResult ActionGateway::validate(const GameContext& ctx) const
{
	// Stage 1: run the inner action's own validation.
	ValidationResult inner_result = _inner->validate(ctx);
	if (!inner_result.valid())
		return inner_result;

	// Stage 2: run the pre-check callback if supplied.
	if (_pre_check)
	{
		// Populate event_type and action_id into the payload snapshot.
		FlowRulesPayload p = _payload;
		p.action_id  = _inner->id();
		p.event_type = EVT_ACTION_SUBMITTED;
		return _pre_check(p);
	}

	return ValidationResult::ok();
}

ActionResult ActionGateway::execute(GameContext& ctx)
{
	ActionResult result = _inner->execute(ctx);

	// Post-hook: purely informational, cannot alter the result.
	if (_post_hook)
	{
		FlowRulesPayload p = _payload;
		p.action_id  = _inner->id();
		p.event_type = result.succeeded()
		                   ? EVT_ACTION_COMPLETED
		                   : EVT_ACTION_FAILED;
		_post_hook(p, result);
	}

	return result;
}

// ── IAction delegation ────────────────────────────────────────────────────────

ActionId     ActionGateway::id()           const { return _inner->id();            }
ActorId      ActionGateway::owner()        const { return _inner->owner();         }
ActionStatus ActionGateway::status()       const { return _inner->status();        }
bool         ActionGateway::is_async()     const { return _inner->is_async();      }
bool         ActionGateway::requires_turn()const { return _inner->requires_turn(); }
bool         ActionGateway::is_multi_step()const { return _inner->is_multi_step(); }

} // namespace gmFlow
