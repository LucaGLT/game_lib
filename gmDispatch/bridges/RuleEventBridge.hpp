#ifndef GMDISPATCH_RULEEVENTBRIDGE_HPP
#define GMDISPATCH_RULEEVENTBRIDGE_HPP

/**
 * @file bridges/RuleEventBridge.hpp
 * @brief Adapter that forwards gmRules rule events into the gmDispatch bus.
 */

#include "Dispatcher.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace gmRules {
struct RuleEvent;
}

namespace gmDispatch {

/**
 * @brief Bridges gmRules rule events to the dispatcher bus.
 */
class RuleEventBridge {
public:
	explicit RuleEventBridge(GmDispatcher& bus);

	void dispatch(const gmRules::RuleEvent& event,
				  int rule_priority = 0);

	void dispatch_many(const std::vector<gmRules::RuleEvent>& events,
					   int rule_priority = 0);

	std::size_t success_count() const;
	std::size_t failure_count() const;
	const std::string& last_error() const;

private:
	GmDispatcher& _bus;
	std::size_t   _success_count;
	std::size_t   _failure_count;
	std::string   _last_error;

	static std::string map_channel(const std::string& event_type);
	static Envelope build_envelope(const gmRules::RuleEvent& event,
								 int rule_priority);
};

} // namespace gmDispatch

#endif // GMDISPATCH_RULEEVENTBRIDGE_HPP