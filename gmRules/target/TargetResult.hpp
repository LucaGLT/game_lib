#ifndef GMRULES_TARGET_TARGETRESULT_HPP
#define GMRULES_TARGET_TARGETRESULT_HPP

/**
 * @file target/TargetResult.hpp
 * @brief Result returned by `TargetResolver::resolve()`.
 */

#include "gmRules/target/TargetRef.hpp"

#include <string>
#include <vector>

namespace gmRules {

/**
 * @brief Outcome of a target resolution.
 *
 * A result is valid if at least one target was resolved (or if the
 * `TargetSpec::required` flag is false and zero targets is acceptable).
 */
class TargetResult
{
public:
    // ── Factory methods ───────────────────────────────────────────────────────

    /** @brief Constructs a successful result containing `targets`. */
    static TargetResult success(std::vector<TargetRef> targets);

    /**
     * @brief Constructs a failure result with a human-readable message.
     * @param message Description of why resolution failed.
     */
    static TargetResult failure(std::string message);

    // ── Queries ───────────────────────────────────────────────────────────────

    /** @brief Returns `true` if resolution succeeded. */
    bool valid() const;

    /** @brief Returns the list of resolved targets (may be empty on failure). */
    const std::vector<TargetRef>& targets() const;

    /** @brief Returns the failure message (empty on success). */
    const std::string& message() const;

private:
    bool                 valid_ = false;
    std::vector<TargetRef> targets_;
    std::string          message_;

    TargetResult(bool valid, std::vector<TargetRef> targets, std::string message);
};

} // namespace gmRules

#endif // GMRULES_TARGET_TARGETRESULT_HPP
