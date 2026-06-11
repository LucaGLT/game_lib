/**
 * @file modifiers/Modifier.cpp
 * @brief Implementation of apply_modifiers().
 */

#include "gmActor/modifiers/Modifier.hpp"

namespace gmActor {

double apply_modifiers(double base_value,
                       const std::string& stat_key,
                       const std::vector<ModifierInstance>& modifiers)
{
    // TODO Phase 4: implement SET → ADD/SUBTRACT → MULTIPLY evaluation.
    (void)stat_key;
    (void)modifiers;
    return base_value;
}

} // namespace gmActor
