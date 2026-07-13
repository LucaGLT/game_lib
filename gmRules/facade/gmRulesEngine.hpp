#ifndef GMRULES_FACADE_GMRULESENGINE_HPP
#define GMRULES_FACADE_GMRULESENGINE_HPP

/**
 * @file facade/gmRulesEngine.hpp
 * @brief Top-level façade for the gmRules library.
 *
 * `gmRulesEngine` composes `TargetResolver`, `ConditionEvaluator`,
 * `EffectResolver`, `StatusEngine`, and `RuleBook`.  Game-specific code should
 * interact with `gmRules` through this class.
 *
 * ## Typical usage
 * @code
 *   MyRuleContext ctx(game_state, event_bus);
 *   gmRules::gmRulesEngine engine;
 *
 *   // Load rule definitions from JSON once at startup.
 *   engine.load_rules_json("dominion_rules.json");
 *
 *   // Resolve a named rule by ID.
 *   auto result = engine.resolve_rule("r_add_action_1", actor_id, targets, ctx);
 *   if (!result.succeeded()) {
 *       // handle failure
 *   }
 * @endcode
 */

#include "gmRules/target/TargetSpec.hpp"
#include "gmRules/target/TargetRef.hpp"
#include "gmRules/target/TargetResult.hpp"
#include "gmRules/target/TargetResolver.hpp"
#include "gmRules/condition/ConditionSpec.hpp"
#include "gmRules/condition/ConditionEvaluator.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/effect/EffectResult.hpp"
#include "gmRules/effect/EffectResolver.hpp"
#include "gmRules/status/StatusDefinition.hpp"
#include "gmRules/status/StatusEngine.hpp"
#include "gmRules/core/RuleResult.hpp"
#include "gmRules/core/RuleContext.hpp"
#include "gmRules/core/RuleBook.hpp"
#include "gmRules/core/Ids.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Top-level façade for the gmRules rule toolkit.
 *
 * Composes TargetResolver, ConditionEvaluator, EffectResolver, StatusEngine.
 */
class gmRulesEngine
{
public:
    // ── Target ────────────────────────────────────────────────────────────────

    /**
     * @brief Resolves a target specification.
     *
     * Side-effect-free — delegates to `TargetResolver`.
     */
    TargetResult resolve_target(const TargetSpec& spec,
                                const ActorId& source_actor_id,
                                const std::vector<TargetRef>& selected_targets,
                                const RuleContext& ctx);

    // ── Conditions ────────────────────────────────────────────────────────────

    /**
     * @brief Evaluates a single condition.
     *
     * Side-effect-free — delegates to `ConditionEvaluator`.
     */
    RuleResult evaluate_condition(const ConditionSpec& spec,
                                  const RuleContext& ctx);

    /**
     * @brief Evaluates all conditions as an implicit ALL_OF.
     *
     * Side-effect-free — delegates to `ConditionEvaluator`.
     */
    RuleResult evaluate_conditions(const std::vector<ConditionSpec>& specs,
                                   const RuleContext& ctx);

    // ── Effects ───────────────────────────────────────────────────────────────

    /**
     * @brief Resolves and applies one effect.
     *
     * Delegates to `EffectResolver`.
     */
    EffectResult resolve_effect(const EffectSpec& spec,
                                const ActorId& source_actor_id,
                                const std::vector<TargetRef>& selected_targets,
                            RuleContext& ctx,
                            int rule_priority = 100);

    /**
     * @brief Resolves and applies multiple effects in sequence.
     *
     * Delegates to `EffectResolver`.
     */
    EffectResult resolve_effects(const std::vector<EffectSpec>& specs,
                                 const ActorId& source_actor_id,
                                 const std::vector<TargetRef>& selected_targets,
                             RuleContext& ctx,
                             int rule_priority = 100);

    // ── Status ────────────────────────────────────────────────────────────────

    /**
     * @brief Applies a status definition to an actor.
     *
     * Delegates to `StatusEngine`.
     */
    RuleResult apply_status(const StatusDefinition& status,
                            const ActorId& owner_actor_id,
                            const std::string& source_id,
                            RuleContext& ctx);

    // ── RuleBook ──────────────────────────────────────────────────────────────

    /**
     * @brief Loads rule definitions from a JSON file into the internal RuleBook.
     *
     * Can be called multiple times to load additional files;
     * definitions are accumulated (not replaced).
     *
     * @param path  Path to the JSON rule definitions file.
     * @throws ERuleBookError if the file cannot be opened or is malformed.
     */
    void load_rules_json(const std::string& path);

    /**
     * @brief Loads rule definitions from an in-memory JSON string.
     *
     * Useful for tests without touching the file system.
     *
     * @param json_text  JSON content as a string.
     * @throws ERuleBookError on parse or registration failure.
     */
    void load_rules_json_string(const std::string& json_text);

    /**
     * @brief Returns direct access to the internal `RuleBook` for advanced use.
     *
     * Use `load_rules_json()` and `resolve_rule()` for the normal workflow.
     * This accessor is provided for cases where direct `RuleBook` API is needed
     * (e.g. pre-registering `RuleDefinition` objects in tests).
     */
    RuleBook& rule_book() { return rule_book_; }

    /**
     * @brief Read-only access to the internal `RuleBook`.
     */
    const RuleBook& rule_book() const { return rule_book_; }

    /**
     * @brief Removes a single rule definition by ID.
     *
     * Delegates to `RuleBook::remove_rule()`.  No-op if the rule is not
     * registered.
     *
     * @param rule_id  Identifier of the rule to remove.
     * @return         @c true if the rule was found and removed.
     */
    bool remove_rule(const RuleId& rule_id);

    /**
     * @brief Replaces an existing rule with a new definition.
     *
     * Delegates to `RuleBook::replace_rule()`.  If the rule ID is not yet
     * registered the definition is simply added.
     *
     * @param def  New `RuleDefinition` (must have a non-empty `rule_id`).
     * @throws ERuleBookError if `def.rule_id` is empty.
     */
    void replace_rule(const RuleDefinition& def);

    /**
     * @brief Removes all registered rules.
     *
     * Use this together with `load_rules_json()` to implement full hot-reload:
     * @code
     *   engine.clear_rules();
     *   engine.load_rules_json("updated_rules.json");
     * @endcode
     */
    void clear_rules();
    RuleResult resolve_rule(const RuleId&                  rule_id,
                            const ActorId&                 source_actor_id,
                            const std::vector<TargetRef>&  selected_targets,
                            RuleContext&                   ctx);

    /**
     * @brief Resolves an ordered list of named rules in sequence.
     *
     * Stops on the first failure.
     *
     * @param rule_ids         Rules to resolve in order.
     * @param source_actor_id  Actor triggering the rules.
     * @param selected_targets Pre-selected targets.
     * @param ctx              Mutable game-state adapter.
     * @return                 Combined `RuleResult`.
     */
    RuleResult resolve_rules(const std::vector<RuleId>&     rule_ids,
                             const ActorId&                 source_actor_id,
                             const std::vector<TargetRef>&  selected_targets,
                             RuleContext&                   ctx);

private:
    TargetResolver     target_resolver_;
    ConditionEvaluator condition_evaluator_;
    EffectResolver     effect_resolver_;
    StatusEngine       status_engine_;
    RuleBook           rule_book_;
};

} // namespace gmRules

#endif // GMRULES_FACADE_GMRULESENGINE_HPP
