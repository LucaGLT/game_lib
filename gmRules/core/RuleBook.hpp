#ifndef GMRULES_CORE_RULEBOOK_HPP
#define GMRULES_CORE_RULEBOOK_HPP

/**
 * @file core/RuleBook.hpp
 * @brief Registry of RuleDefinitions with integrated effect resolution.
 *
 * `RuleBook` is the **missing link** between `RuleGroupRegistry` (which knows
 * *which* rules are active by ID) and `EffectResolver` (which knows *how* to
 * execute an `EffectSpec`).
 *
 * ## Typical game loop
 * @code
 *   // 1. Load definitions once at startup.
 *   gmRules::RuleBook book;
 *   RuleBookLoader::load_json("dominion_rules.json", book);
 *
 *   // 2. After a card zone-change activates a rule group:
 *   for (const RuleId& rid : registry.active_rule_ids())
 *   {
 *       RuleResult r = book.resolve_rule(rid, actor_id, targets, ctx);
 *       if (!r.succeeded()) { /* log or skip *\/ }
 *   }
 * @endcode
 *
 * ## Thread safety
 * Not thread-safe.  All calls must originate from the same thread or be
 * externally synchronized.
 *
 * ## Error handling
 * - `ERuleBookError` — subclass of `ERulesError` — is thrown only on
 *   structural violations (duplicate `rule_id`, etc.).
 * - Resolution failures (condition not met, effect failed) are returned as
 *   `RuleResult::fail(...)` rather than thrown.
 */

#include "gmRules/core/RuleDefinition.hpp"
#include "gmRules/core/RuleResult.hpp"
#include "gmRules/core/RuleTypes.hpp"
#include "gmRules/effect/EffectResolver.hpp"
#include "gmRules/condition/ConditionEvaluator.hpp"
#include "gmRules/target/TargetRef.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace gmRules {

// ─────────────────────────────────────────────────────────────────────────────
// Exception
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thrown when a `RuleBook` operation encounters a structural problem.
 *
 * Examples: registering a duplicate `rule_id`, resolving an unknown `rule_id`.
 */
class ERuleBookError : public ERulesError
{
public:
	explicit ERuleBookError(const std::string& msg) : ERulesError(msg) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// RuleBook
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class RuleBook
 * @brief Owns all `RuleDefinition` objects and resolves them on demand.
 *
 * Internally wraps a `ConditionEvaluator` and an `EffectResolver`.
 * No external state is stored between calls.
 */
class RuleBook
{
public:
	RuleBook()  = default;
	~RuleBook() = default;

	RuleBook(const RuleBook&)            = delete;
	RuleBook& operator=(const RuleBook&) = delete;

	RuleBook(RuleBook&&)            = default;
	RuleBook& operator=(RuleBook&&) = default;

	// ── Registration ─────────────────────────────────────────────────────────

	/**
	 * @brief Registers a rule definition.
	 *
	 * The `def.rule_id` must be non-empty and not already registered.
	 *
	 * @param def Definition to register (copied by value).
	 * @throws ERuleBookError if `def.rule_id` is empty or already registered.
	 */
	void register_rule(const RuleDefinition& def);

	/**
	 * @brief Returns `true` if a definition with the given ID has been registered.
	 */
	bool has_rule(const RuleId& rule_id) const;

	/**
	 * @brief Returns the definition registered under `rule_id`.
	 *
	 * @throws ERuleBookError if `rule_id` is not registered.
	 */
	const RuleDefinition& get_rule(const RuleId& rule_id) const;

	/**
	 * @brief Returns the total number of registered rules.
	 */
	int rule_count() const;

	/**
	 * @brief Removes the rule with the given ID from the book.
	 *
	 * If the rule does not exist, the call is a no-op (no exception).
	 *
	 * @param rule_id  ID of the rule to remove.
	 * @return         @c true if the rule was found and removed, @c false if it
	 *                 was not registered.
	 */
	bool remove_rule(const RuleId& rule_id);

	/**
	 * @brief Replaces an existing rule with a new definition.
	 *
	 * Equivalent to calling @c remove_rule(def.rule_id) followed by
	 * @c register_rule(def).  If the rule is not currently registered, the
	 * new definition is simply added (same as @c register_rule).
	 *
	 * @param def  New definition (must have a non-empty @c rule_id).
	 * @throws ERuleBookError if @c def.rule_id is empty.
	 */
	void replace_rule(const RuleDefinition& def);

	/**
	 * @brief Removes all registered rules from the book.
	 *
	 * After this call @c rule_count() == 0.
	 * Useful for full hot-reload: call @c clear_rules() then
	 * @c register_rule() / @c load_json() again.
	 */
	void clear_rules();

	// ── Resolution ───────────────────────────────────────────────────────────

	/**
	 * @brief Evaluates preconditions and applies all effects for one rule.
	 *
	 * Resolution steps:
	 * 1. Look up the `RuleDefinition` for `rule_id`.
	 * 2. Evaluate `def.preconditions` — returns failure if any fails.
	 * 3. Apply `def.effects` via `EffectResolver::resolve_many()`.
	 * 4. Return the aggregate `RuleResult`.
	 *
	 * A rule with no preconditions always fires (step 2 is a no-op).
	 * A rule with no effects succeeds immediately (step 3 is a no-op).
	 *
	 * @param rule_id          Rule to resolve.
	 * @param source_actor_id  Actor triggering the rule.
	 * @param selected_targets Pre-selected targets forwarded to TargetResolver.
	 * @param ctx              Mutable game-state adapter.
	 * @return                 `RuleResult::ok()` or `RuleResult::fail(...)`.
	 * @throws ERuleBookError  if `rule_id` is not registered.
	 */
	RuleResult resolve_rule(const RuleId&                  rule_id,
	                        const ActorId&                 source_actor_id,
	                        const std::vector<TargetRef>&  selected_targets,
	                        RuleContext&                   ctx) const;

	/**
	 * @brief Resolves a list of rules in sequence.
	 *
	 * Stops and returns the first failure if the failed rule has
	 * `EffectSpec::stop_on_failure` set to `true` in all its effects.
	 * Unregistered IDs in `rule_ids` throw `ERuleBookError`.
	 *
	 * @param rule_ids         Ordered list of rules to resolve.
	 * @param source_actor_id  Actor triggering the rules.
	 * @param selected_targets Pre-selected targets.
	 * @param ctx              Mutable game-state adapter.
	 * @return                 Combined `RuleResult`.
	 */
	RuleResult resolve_rules(const std::vector<RuleId>&     rule_ids,
	                         const ActorId&                 source_actor_id,
	                         const std::vector<TargetRef>&  selected_targets,
	                         RuleContext&                   ctx) const;

private:
	std::unordered_map<RuleId, RuleDefinition> _definitions;
	ConditionEvaluator                         _condition_evaluator;
	EffectResolver                             _effect_resolver;
};

} // namespace gmRules

#endif // GMRULES_CORE_RULEBOOK_HPP
