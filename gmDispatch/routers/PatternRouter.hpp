#ifndef GMDISPATCH_PATTERNROUTER_HPP
#define GMDISPATCH_PATTERNROUTER_HPP

/**
 * @file routers/PatternRouter.hpp
 * @brief Wildcard pattern-matching router with targeted delivery — Phase 4.
 */

#include "IRouter.hpp"
#include "IChannel.hpp"
#include "Envelope.hpp"

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace gmDispatch {

/**
 * @brief Router supporting wildcard subscription patterns and targeted delivery.
 *
 * PatternRouter extends the routing capabilities of @ref SyncRouter with two
 * Phase 4 features:
 *
 * ### 1. Wildcard pattern matching
 * Subscription keys may now be:
 * | Pattern          | Matches                                     |
 * |------------------|---------------------------------------------|
 * | `"engine.tick"`  | Exact — only `typeId == "engine.tick"`      |
 * | `"engine.*"`     | Prefix wildcard — any `"engine.X"` typeId   |
 * | `"*"`            | Broadcast — every typeId                    |
 *
 * Only a single trailing `*` after a `.` is supported.  Nested wildcards
 * (e.g. `"*.tick"`) are not supported in V1.
 *
 * ### 2. Targeted delivery
 * When @c Envelope::targets is **non-empty**, a channel receives the envelope
 * only if **at least one** of the following is true:
 * - @c IChannel::name() is empty (anonymous channel — always receives).
 * - @c IChannel::name() appears in @c Envelope::targets.
 *
 * When @c Envelope::targets is empty, all matched channels receive the
 * envelope (broadcast behaviour — same as @ref SyncRouter).
 *
 * ### Thread safety
 * No internal mutex.  The owning dispatcher acquires a lock before calling
 * any @c IRouter method.
 *
 * @par Example
 * @code
 *   std::unique_ptr<gmDispatch::PatternRouter> router =
 *       std::make_unique<gmDispatch::PatternRouter>();
 *
 *   // Subscribe to all engine subsystem events
 *   router->subscribe("engine.*", uiChannel);
 *
 *   // Subscribe to all events
 *   router->subscribe("*", diagnosticsChannel);
 * @endcode
 */
class PatternRouter : public IRouter {
public:
	PatternRouter() = default;

	/**
	 * @brief Registers @p channel for envelopes matching @p pattern.
	 *
	 * @param pattern Subscription pattern: exact, prefix wildcard, or @c "*".
	 * @param channel Channel to register.
	 */
	void subscribe(const std::string&        pattern,
				   std::shared_ptr<IChannel> channel) override;

	/**
	 * @brief Removes the first occurrence of @p channel under @p pattern.
	 *
	 * @param pattern The pattern used at subscription time.
	 * @param channel The channel to remove.
	 */
	void unsubscribe(const std::string&        pattern,
					 std::shared_ptr<IChannel> channel) override;

	/**
	 * @brief Routes @p envelope to all channels matching both pattern and targets.
	 *
	 * @param envelope The event to route.
	 */
	void route(const Envelope& envelope) override;

	/**
	 * @brief Flushes every unique registered channel exactly once.
	 */
	void flush() override;

private:
	/**
	 * @brief Returns @c true if @p pattern matches @p typeId.
	 *
	 * @param pattern Subscription pattern.
	 * @param typeId  Envelope typeId to test.
	 */
	static bool match_pattern(const std::string& pattern,
							  const std::string& typeId);

	/**
	 * @brief Returns @c true if @p channel should receive @p envelope.
	 *
	 * Applies targeted-delivery logic:
	 * - If @c envelope.targets is empty → always true (broadcast).
	 * - If @c channel.name() is empty   → always true (anonymous).
	 * - Otherwise: true iff @c channel.name() is in @c envelope.targets.
	 */
	static bool is_targeted(const std::shared_ptr<IChannel>& channel,
							const Envelope&                  envelope);

	/// Subscription map: pattern → ordered list of subscribed channels.
	std::map<std::string, std::vector<std::shared_ptr<IChannel>>> _routes;
};

} // namespace gmDispatch

#endif // GMDISPATCH_PATTERNROUTER_HPP
