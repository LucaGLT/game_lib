#ifndef GMRULES_TARGET_TARGETREF_HPP
#define GMRULES_TARGET_TARGETREF_HPP

/**
 * @file target/TargetRef.hpp
 * @brief A resolved reference to a specific target.
 */

#include "gmRules/target/TargetSpec.hpp"

#include <string>

namespace gmRules {

/**
 * @brief A resolved reference to one specific target.
 *
 * `TargetRef` is the runtime output of `TargetResolver::resolve()`.
 * It pairs a `TargetKind` with the string ID of the resolved entity.
 */
struct TargetRef
{
    TargetKind  kind = TargetKind::NONE; ///< Domain of the resolved target
    std::string id;                      ///< ID of the resolved entity
};

} // namespace gmRules

#endif // GMRULES_TARGET_TARGETREF_HPP
