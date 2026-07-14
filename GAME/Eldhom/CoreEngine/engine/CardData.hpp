#ifndef ELDHOM_ENGINE_CARDDATA_HPP
#define ELDHOM_ENGINE_CARDDATA_HPP

/**
 * @file engine/CardData.hpp
 * @brief POD types for hero action card definitions.
 *
 * `EldhomCard` holds static card data loaded from `cards_base.json`.
 * It does NOT embed gmAlea types in the struct layout so that forward
 * declarations remain clean; `card_type_raw` carries the string from JSON
 * which is converted to `gmAlea::CardType` when the catalog is built.
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp"
#include "gmAlea/CardType.hpp"

#include <string>
#include <vector>

namespace eldhom {

// ─────────────────────────────────────────────────────────────────────────────
// EldhomEffect  (one effect step on a card)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct EldhomEffect
 * @brief Single effect entry on an action card or behavior step.
 *
 * `effect_type` uses the string constants defined in `EffectStrings`
 * (see rules/EldhomRuleAdapter.hpp).
 */
struct EldhomEffect {
	EffectType  effect_type;       ///< "DAMAGE", "MOVE", "HEAL", "FORMATION_PUSH", …
	int         amount    = 0;     ///< Numeric magnitude of the effect
	std::string target;            ///< Target selector: "ENEMY_FRONTLINE", "SELF", …
	/// Attack medium for DAMAGE/DEAL_DAMAGE effects: "MELEE" (default) or
	/// "RANGED". Declarative today (not yet enforced, e.g. no check that a
	/// RANGED attacker must be in RETROGUARDIA); paired with `range` below.
	std::string attack_type = "MELEE";
	/// Optional guard. Only `"IF_BOTH_FRONTLINE"` is actually evaluated today
	/// (by `EldhomEngine::play_card()`, for a `DISCARD_THEN_DRAW` effect on a
	/// DAMAGE card — e.g. Colpo Secco: applies only if both attacker and
	/// target are in FRONTLINE). Any other value is currently ignored.
	std::string condition;
	std::string value;             ///< Extra payload (e.g. status tag name)
	/// Generic attack range in location-hops for DAMAGE/DEAL_DAMAGE effects:
	/// 0 = mischia (same location), 1 = adjacent location, 2 = adjacent-of-
	/// adjacent, etc. Resolved via BFS on the mission's adjacency graph.
	int         range     = 0;
	/// For MOVE effects: if true, the path may not cross through (as an
	/// intermediate step) any location occupied by enemies of the caster's
	/// faction. The final destination is exempt (Formazione is resolved there
	/// normally). Used by Passo Cauto / Scatto Breve.
	bool        avoid_enemy_locations = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// EldhomCard
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct EldhomCard
 * @brief Static definition of a hero action card.
 */
struct EldhomCard {
	CardId                   card_id;
	std::string              name;
	std::string              origin;          ///< "BASE", "THAEL", "VELYR", …
	gmAlea::CardType         card_type  = gmAlea::CardType::SINGLE;
	int                      timeline_cost = 2;
	std::vector<EldhomEffect> effects;
	std::string              reaction_trigger; ///< Empty means no instant reaction
	/// True if the caster must be in FRONTLINE to play this card (e.g. Fendente
	/// Pesante, Spinta di Corpo). Checked in EldhomEngine::play_card() before
	/// any effect is applied.
	bool                     requires_frontline = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// Location definition (static)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct LocationDef
 * @brief Static definition of a mission location.
 */
struct LocationDef {
	LocationId              id;
	std::string             name;
	std::vector<LocationId> adjacent; ///< IDs of directly reachable locations
};

} // namespace eldhom

#endif // ELDHOM_ENGINE_CARDDATA_HPP
