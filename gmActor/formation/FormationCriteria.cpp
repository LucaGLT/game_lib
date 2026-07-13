/**
 * @file formation/FormationCriteria.cpp
 * @brief Implementation of built-in FormationCriterion factory functions.
 */

#include "gmActor/formation/FormationCriteria.hpp"
#include "gmActor/actors/ActorStateCommon.hpp"
#include "gmActor/core/Errors.hpp"

#include <stdexcept>

namespace gmActor {
namespace FormationCriteria {

// ── by_highest_hp ─────────────────────────────────────────────────────────────

FormationCriterion by_highest_hp()
{
	return [](const ActorId& a, const ActorId& b, const ActorStore& store) -> bool
	{
		// Safe access: some actor types (MonsterGroup) have no common state.
		// If access fails, treat both actors as equal for this criterion.
		int hp_a = 0;
		int hp_b = 0;
		try { hp_a = store.common(a).current_hp; } catch (const std::exception&) {}
		try { hp_b = store.common(b).current_hp; } catch (const std::exception&) {}
		return hp_a > hp_b;
	};
}

// ── by_lowest_timeline ────────────────────────────────────────────────────────

FormationCriterion by_lowest_timeline()
{
	return [](const ActorId& a, const ActorId& b, const ActorStore& store) -> bool
	{
		int pos_a = 0;
		int pos_b = 0;
		try { pos_a = store.common(a).timeline_position; } catch (const std::exception&) {}
		try { pos_b = store.common(b).timeline_position; } catch (const std::exception&) {}
		return pos_a < pos_b;
	};
}

// ── random ────────────────────────────────────────────────────────────────────

FormationCriterion random(unsigned seed)
{
	// Simple LCG parameters (Knuth / MMIX).
	// The hash of the two IDs is combined with the seed so the same pair
	// always produces the same order for a given seed, making the result
	// deterministic and reproducible.
	return [seed](const ActorId& a, const ActorId& b, const ActorStore&) -> bool
	{
		// Combine string hashes with the seed using the LCG.
		auto lcg = [](unsigned x) -> unsigned
		{
			// LCG: x = x * 6364136223846793005 (mod 2^32) + 1442695040888963407
			// Reduced to 32-bit arithmetic.
			return x * 1664525u + 1013904223u;
		};

		std::size_t hash_a = std::hash<std::string>{}(a);
		std::size_t hash_b = std::hash<std::string>{}(b);

		unsigned val_a = lcg(seed ^ static_cast<unsigned>(hash_a));
		unsigned val_b = lcg(seed ^ static_cast<unsigned>(hash_b));

		return val_a < val_b;
	};
}

} // namespace FormationCriteria
} // namespace gmActor
