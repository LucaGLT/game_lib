#ifndef GMRULES_EFFECT_EFFECTRESULT_HPP
#define GMRULES_EFFECT_EFFECTRESULT_HPP

/**
 * @file effect/EffectResult.hpp
 * @brief Result returned by `EffectResolver::resolve()`.
 *
 * An `EffectResult` carries:
 * - Success / partial-success / failure status
 * - `RuleEvent` objects emitted during resolution
 * - Warning messages for optional-effect failures
 */

#include "gmRules/core/RuleEvent.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Outcome of resolving one or more `EffectSpec` objects.
 */
class EffectResult
{
public:
    // ── Factory methods ───────────────────────────────────────────────────────

    /** @brief Constructs a clean success result. */
    static EffectResult success();

    /**
     * @brief Constructs a partial-success result with warning messages.
     * @param warnings Non-fatal issues encountered during resolution.
     */
    static EffectResult partial(std::vector<std::string> warnings);

    /**
     * @brief Constructs a failure result.
     * @param message Human-readable failure description.
     */
    static EffectResult failure(std::string message);

    // ── Status queries ────────────────────────────────────────────────────────

    /** @brief Returns `true` if the effect resolved without hard failures. */
    bool succeeded() const;

    /** @brief Returns `true` if there were non-fatal warnings. */
    bool partial_success() const;

    // ── Accessors ─────────────────────────────────────────────────────────────

    /** @brief Events emitted during resolution. */
    const std::vector<RuleEvent>& events() const;

    /** @brief Warning messages for optional-effect failures. */
    const std::vector<std::string>& warnings() const;

    /** @brief Failure message (empty on success or partial). */
    const std::string& message() const;

    // ── Mutation (called by EffectResolver) ───────────────────────────────────

    /** @brief Appends an emitted event to the result. */
    void add_event(RuleEvent event);

    /** @brief Appends a warning to the result. */
    void add_warning(std::string warning);

private:
    bool succeeded_      = true;
    bool partial_        = false;
    std::vector<RuleEvent>   events_;
    std::vector<std::string> warnings_;
    std::string message_;

    EffectResult(bool succeeded, bool partial, std::string message);
};

} // namespace gmRules

#endif // GMRULES_EFFECT_EFFECTRESULT_HPP
