#ifndef GMALEA_BRIDGES_GMALEAGMRULES_CARDRULEBRIDGE_HPP
#define GMALEA_BRIDGES_GMALEAGMRULES_CARDRULEBRIDGE_HPP

/**
 * @file bridges/gmAlea_gmRules/CardRuleBridge.hpp
 * @brief Bridge that wires GmCompDeck zone-change events to RuleGroupRegistry.
 *
 * This header is the **only** file in the entire `gmAlea` tree that is allowed
 * to `#include` a `gmRules` header.  All other `gmAlea` sources remain
 * independent of `gmRules`.
 *
 * ## Purpose
 *
 * `CardRuleBridge` implements the observer contract by forwarding
 * `GmCompDeck::ZoneChangeCallback` events to `RuleGroupRegistry::activate()`
 * and `RuleGroupRegistry::deactivate()`.
 *
 * The activation logic is:
 *   - Token enters `PLAY_AREA` or `MEMORY` → `activate(rule_group_id)` if
 *     the group is registered and lifecycle is `TRANSIENT` (or the group is
 *     not yet active for `PERSISTENT` / `TRIGGER_BOUND`).
 *   - Token leaves `PLAY_AREA` or `MEMORY` going to any other zone →
 *     `deactivate(rule_group_id)` if lifecycle is `TRANSIENT`.
 *
 * ## Usage
 * @code
 *   #include "gmAlea/bridges/gmAlea_gmRules/CardRuleBridge.hpp"
 *
 *   gmAlea::GmCompDeck       deck("Player1", {101, 102, 103});
 *   gmRules::RuleGroupRegistry reg;
 *
 *   // Register the rule group.
 *   gmRules::RuleGroup rg;
 *   rg.group_id  = "rg_village";
 *   rg.rule_ids  = {"r_add_buy"};
 *   rg.lifecycle = gmRules::RuleGroupLifecycle::TRANSIENT;
 *   reg.register_group(rg);
 *
 *   // Wire the bridge.
 *   deck.register_rule_group(101, "rg_village");
 *   gmAlea::CardRuleBridge::attach(deck, reg);
 *
 *   deck.draw_to_hand(1);   // no activation yet
 *   deck.play_card(101);    // → reg.activate("rg_village")
 *   deck.resolve_card(101); // → reg.deactivate("rg_village")
 * @endcode
 */

#include "../../GmCompDeck.hpp"
#include "../../../gmRules/core/RuleGroupRegistry.hpp"

namespace gmAlea {

// ─────────────────────────────────────────────────────────────────────────────
// Active-zone predicate
// ─────────────────────────────────────────────────────────────────────────────

namespace detail {

/// Returns true for zones that are considered "card is in active play".
inline bool is_active_zone(ZoneId z) noexcept
{
	return z == ZoneId::PLAY_AREA || z == ZoneId::MEMORY;
}

} // namespace detail

// ─────────────────────────────────────────────────────────────────────────────
// CardRuleBridge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class CardRuleBridge
 * @brief Static helper that attaches a `ZoneChangeCallback` to a `GmCompDeck`
 *        and routes events to a `gmRules::RuleGroupRegistry`.
 *
 * All methods are static.  No instance is needed.
 */
class CardRuleBridge
{
public:
	CardRuleBridge()  = delete;
	~CardRuleBridge() = delete;

	/**
	 * @brief Attaches the bridge callback to `deck`, routing to `registry`.
	 *
	 * The callback captures `registry` by reference.  Callers must ensure
	 * `registry` outlives the `deck` or call `detach()` before either is
	 * destroyed.
	 *
	 * @param deck     Deck to observe.
	 * @param registry Registry to notify.
	 */
	static void attach(GmCompDeck& deck, gmRules::RuleGroupRegistry& registry)
	{
		deck.set_zone_change_callback(
			[&registry](uint32_t           /*token_id*/,
			            const std::string& rule_group_id,
			            ZoneId             from,
			            ZoneId             to)
			{
				if (rule_group_id.empty())
				{
					return; // no rule group linked to this token
				}
				if (!registry.is_registered(rule_group_id))
				{
					return; // unknown group — ignore silently
				}

				const bool entering_active = detail::is_active_zone(to);
				const bool leaving_active  = detail::is_active_zone(from)
				                             && !detail::is_active_zone(to);

				if (entering_active)
				{
					registry.activate(rule_group_id);
				}
				else if (leaving_active && registry.is_transient(rule_group_id))
				{
					registry.deactivate(rule_group_id);
				}
			}
		);
	}

	/**
	 * @brief Detaches the bridge callback from `deck`.
	 *
	 * Replaces the stored callback with an empty one.
	 *
	 * @param deck Deck from which to remove the callback.
	 */
	static void detach(GmCompDeck& deck)
	{
		deck.set_zone_change_callback(ZoneChangeCallback{});
	}
};

} // namespace gmAlea

#endif // GMALEA_BRIDGES_GMALEAGMRULES_CARDRULEBRIDGE_HPP
