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
	double result = base_value;
	bool   set_applied = false;

	// Pass 1: SET — last SET wins (D13)
	for (const ModifierInstance& m : modifiers)
	{
		if (m.stat_key != stat_key) continue;
		if (m.operation == ModifierOperation::SET)
		{
			result = m.value;
			set_applied = true;
		}
	}

	// Pass 2: ADD / SUBTRACT
	for (const ModifierInstance& m : modifiers)
	{
		if (m.stat_key != stat_key) continue;
		if (m.operation == ModifierOperation::ADD)
		{
			result += m.value;
		}
		else if (m.operation == ModifierOperation::SUBTRACT)
		{
			result -= m.value;
		}
	}

	// Pass 3: MULTIPLY
	for (const ModifierInstance& m : modifiers)
	{
		if (m.stat_key != stat_key) continue;
		if (m.operation == ModifierOperation::MULTIPLY)
		{
			result *= m.value;
		}
	}

	(void)set_applied;
	return result;
}

} // namespace gmActor
