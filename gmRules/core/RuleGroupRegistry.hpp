#ifndef GMRULES_CORE_RULEGROUPREGISTRY_HPP
#define GMRULES_CORE_RULEGROUPREGISTRY_HPP

/**
 * @file core/RuleGroupRegistry.hpp
 * @brief Registry for named rule groups with lifecycle-aware activation.
 *
 * `RuleGroupRegistry` tracks which `RuleGroup` instances are registered and
 * which are currently active.  It is the single point of truth for the
 * active-rule set at any moment in the game.
 *
 * The registry does **not** execute rules itself — that is the responsibility
 * of the rules-engine layer.  It only manages membership and activation state.
 *
 * ## Thread safety
 * Not thread-safe.  All calls must originate from the same thread, or the
 * caller must provide external synchronization.
 *
 * ## Error handling
 * - `ERuleGroupError` is thrown on structural violations (e.g. duplicate
 *   registration, unknown group ID).
 * - Never thrown for normal activate / deactivate calls on already-active or
 *   already-inactive groups — those are silent no-ops.
 */

#include "RuleGroup.hpp"
#include "RuleTypes.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gmRules {

// ─────────────────────────────────────────────────────────────────────────────
// Exception
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thrown when a `RuleGroupRegistry` operation encounters a
 *        structural problem (e.g. duplicate group ID, unknown group ID).
 */
class ERuleGroupError : public ERulesError
{
public:
	explicit ERuleGroupError(const std::string& msg) : ERulesError(msg) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// RuleGroupRegistry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class RuleGroupRegistry
 * @brief Manages the set of known rule groups and their activation state.
 *
 * ## Typical usage
 * @code
 *   gmRules::RuleGroupRegistry reg;
 *
 *   gmRules::RuleGroup rg;
 *   rg.group_id  = "rg_village";
 *   rg.rule_ids  = {"r_add_buy", "r_add_coin"};
 *   rg.lifecycle = gmRules::RuleGroupLifecycle::TRANSIENT;
 *
 *   reg.register_group(rg);
 *   reg.activate("rg_village");
 *   // … card stays in play …
 *   reg.deactivate("rg_village");
 * @endcode
 */
class RuleGroupRegistry
{
public:
	RuleGroupRegistry()  = default;
	~RuleGroupRegistry() = default;

	RuleGroupRegistry(const RuleGroupRegistry&)            = delete;
	RuleGroupRegistry& operator=(const RuleGroupRegistry&) = delete;

	RuleGroupRegistry(RuleGroupRegistry&&)            = default;
	RuleGroupRegistry& operator=(RuleGroupRegistry&&) = default;

	// ─────────────────────────────────────────────────────────────────────────
	// Registration
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * @brief Registers a new rule group.
	 *
	 * The `group.group_id` must be non-empty and must not already be
	 * registered.  Registered groups start in the **inactive** state.
	 *
	 * @param group Rule group definition to register.
	 * @throws ERuleGroupError if `group.group_id` is empty or already registered.
	 */
	void register_group(const RuleGroup& group);

	/**
	 * @brief Removes a registered rule group.
	 *
	 * If the group is currently active it is deactivated first.
	 * Silently succeeds if `group_id` is not registered.
	 *
	 * @param group_id ID of the group to remove.
	 */
	void unregister_group(const std::string& group_id);

	/**
	 * @brief Returns true if a group with the given ID has been registered.
	 *
	 * @param group_id Group to look up.
	 * @return `true` if registered, `false` otherwise.
	 */
	bool is_registered(const std::string& group_id) const;

	// ─────────────────────────────────────────────────────────────────────────
	// Activation / deactivation
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * @brief Activates the named rule group.
	 *
	 * Silently succeeds if the group is already active.
	 *
	 * @param group_id Group to activate.
	 * @throws ERuleGroupError if `group_id` is not registered.
	 */
	void activate(const std::string& group_id);

	/**
	 * @brief Deactivates the named rule group.
	 *
	 * Silently succeeds if the group is already inactive.
	 *
	 * @param group_id Group to deactivate.
	 * @throws ERuleGroupError if `group_id` is not registered.
	 */
	void deactivate(const std::string& group_id);

	/**
	 * @brief Returns true if the named group is currently active.
	 *
	 * @param group_id Group to query.
	 * @return `true` if active, `false` if inactive or not registered.
	 */
	bool is_active(const std::string& group_id) const;

	// ─────────────────────────────────────────────────────────────────────────
	// Query
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * @brief Returns the flat list of rule IDs currently active.
	 *
	 * Iterates over all active groups and concatenates their `rule_ids` lists
	 * in registration order.  Duplicate rule IDs across groups are preserved.
	 *
	 * @return Ordered list of active rule IDs.
	 */
	std::vector<RuleId> active_rule_ids() const;

	/**
	 * @brief Returns the `RuleGroup` registered under `group_id`.
	 *
	 * @param group_id Group to look up.
	 * @return Const reference to the stored `RuleGroup`.
	 * @throws ERuleGroupError if `group_id` is not registered.
	 */
	const RuleGroup& get_group(const std::string& group_id) const;

	/**
	 * @brief Returns true if the named group has a TRANSIENT lifecycle.
	 *
	 * Convenience helper used by the bridge layer.
	 *
	 * @param group_id Group to check.
	 * @return `true` if registered and lifecycle == TRANSIENT.
	 *         `false` if not registered or lifecycle != TRANSIENT.
	 */
	bool is_transient(const std::string& group_id) const;

	/**
	 * @brief Returns the total number of registered groups.
	 */
	int registered_count() const;

	/**
	 * @brief Returns the total number of active groups.
	 */
	int active_count() const;

private:
	/// All registered groups, keyed by group_id.
	std::unordered_map<std::string, RuleGroup> _registry;

	/// IDs of currently active groups.
	std::unordered_set<std::string>            _active_groups;

	/// Insertion order for registered groups (for deterministic active_rule_ids).
	std::vector<std::string>                   _registration_order;
};

} // namespace gmRules

#endif // GMRULES_CORE_RULEGROUPREGISTRY_HPP
