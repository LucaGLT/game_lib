/**
 * @file tests/MockRuleContext.hpp
 * @brief Minimal in-memory RuleContext implementation for gmRules unit tests.
 *
 * This mock does not depend on gmActor or gmMap — it uses plain
 * std::unordered_map tables so tests remain standalone.
 */

#pragma once
#ifndef GMRULES_TESTS_MOCKRULECONTEXT_HPP
#define GMRULES_TESTS_MOCKRULECONTEXT_HPP

#include "gmRules/core/RuleContext.hpp"
#include "gmRules/effect/EffectSpec.hpp"
#include "gmRules/target/TargetRef.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace gmRules_test {

struct ActorData
{
    int         current_hp = 0;
    int         max_hp     = 0;
    std::string location_id;
    std::string faction; // "heroes" or "monsters"
    std::string team_id;
    bool        alive      = true;
    std::unordered_set<std::string> tags;
    std::vector<gmRules::StatusInstanceId> status_instance_ids;
    std::unordered_map<std::string, int> resources;
    std::unordered_set<std::string> equipped_items;
};

struct LocationData
{
    std::unordered_set<std::string> tags;
    std::vector<std::string> adjacent;
    std::vector<gmRules::ActorId> actors;
    bool passable   = true;
    std::string owner_id;
};

class MockRuleContext : public gmRules::RuleContext
{
public:
    bool allow_extended_effects = false;

    // ── Test-setup helpers ────────────────────────────────────────────────────

    void add_actor(const gmRules::ActorId& id, const std::string& faction,
                   int hp, int max_hp, const std::string& location)
    {
        ActorData d;
        d.current_hp  = hp;
        d.max_hp      = max_hp;
        d.location_id = location;
        d.faction     = faction;
        d.team_id     = faction;
        d.alive       = true;
        actors_[id]   = d;
        locations_[location].actors.push_back(id);
    }

    void add_location(const gmRules::LocationId& id)
    {
        locations_.emplace(id, LocationData{});
    }

    void add_adjacency(const gmRules::LocationId& a, const gmRules::LocationId& b)
    {
        locations_[a].adjacent.push_back(b);
        locations_[b].adjacent.push_back(a);
    }

    void add_deck_zone(const gmRules::DeckId& deck_id,
                       const std::string& zone_name,
                       const std::vector<gmRules::CardId>& cards)
    {
        deck_zones_[deck_id][zone_name] = cards;
        if (!decks_.count(deck_id))
            decks_[deck_id] = cards; // default zone mirrors first zone added
    }

    void add_actor_tag(const gmRules::ActorId& id, const std::string& tag) override
    {
        actors_[id].tags.insert(tag);
    }

    // ── Collected outputs (for test assertions) ───────────────────────────────

    std::vector<gmRules::RuleEvent>    emitted_events;
    std::vector<std::string>           emitted_buses;
    std::vector<gmRules::StatusInstance> added_statuses;
    std::vector<gmRules::StatusInstanceId> removed_statuses;
    std::vector<gmRules::ActorId>      spawned_actors;
    std::vector<gmRules::ActorId>      despawned_actors;
    std::vector<gmRules::ActorId>      revived_actors;
    std::vector<std::string>           created_barriers;
    std::vector<std::string>           removed_barriers;

    // ── RuleContext interface ─────────────────────────────────────────────────

    bool has_actor(const gmRules::ActorId& id) const override
    {
        return actors_.count(id) > 0;
    }

    bool actor_has_tag(const gmRules::ActorId& id,
                       const std::string& tag) const override
    {
        auto it = actors_.find(id);
        if (it == actors_.end()) return false;
        return it->second.tags.count(tag) > 0;
    }

    int actor_current_hp(const gmRules::ActorId& id) const override
    {
        auto it = actors_.find(id);
        return it != actors_.end() ? it->second.current_hp : 0;
    }

    int actor_max_hp(const gmRules::ActorId& id) const override
    {
        auto it = actors_.find(id);
        return it != actors_.end() ? it->second.max_hp : 0;
    }

    int actor_resource(const gmRules::ActorId& id,
                       const std::string& resource_id) const override
    {
        auto it = actors_.find(id);
        if (it == actors_.end()) return 0;
        auto rit = it->second.resources.find(resource_id);
        return rit != it->second.resources.end() ? rit->second : 0;
    }

    bool actor_has_status(const gmRules::ActorId& id,
                          const gmRules::StatusId& status_id) const override
    {
        auto it = actors_.find(id);
        if (it == actors_.end()) return false;
        for (const std::string& iid : it->second.status_instance_ids)
        {
            if (iid.find(status_id) != std::string::npos) return true;
        }
        return false;
    }

    std::vector<gmRules::StatusInstanceId>
    statuses_on_actor(const gmRules::ActorId& id) const override
    {
        auto it = actors_.find(id);
        if (it == actors_.end()) return {};
        return it->second.status_instance_ids;
    }

    bool are_allies(const gmRules::ActorId& a,
                    const gmRules::ActorId& b) const override
    {
        auto ia = actors_.find(a);
        auto ib = actors_.find(b);
        if (ia == actors_.end() || ib == actors_.end()) return false;
        return ia->second.faction == ib->second.faction;
    }

    bool are_enemies(const gmRules::ActorId& a,
                     const gmRules::ActorId& b) const override
    {
        auto ia = actors_.find(a);
        auto ib = actors_.find(b);
        if (ia == actors_.end() || ib == actors_.end()) return false;
        return ia->second.faction != ib->second.faction;
    }

    void modify_actor_hp(const gmRules::ActorId& id, int delta) override
    {
        auto it = actors_.find(id);
        if (it == actors_.end()) return;
        it->second.current_hp += delta;
        if (it->second.current_hp < 0) it->second.current_hp = 0;
        if (it->second.current_hp > it->second.max_hp) it->second.current_hp = it->second.max_hp;
    }

    void remove_actor_tag(const gmRules::ActorId& id,
                          const std::string& tag) override
    {
        auto it = actors_.find(id);
        if (it != actors_.end()) it->second.tags.erase(tag);
    }

    // ── Actor lifecycle (Chapter 4) ───────────────────────────────────────────

    void spawn_actor(const gmRules::ActorId& id,
                     const std::string& /*spec_json*/) override
    {
        if (!actors_.count(id))
        {
            ActorData d;
            d.alive = true;
            actors_[id] = d;
        }
        spawned_actors.push_back(id);
    }

    void despawn_actor(const gmRules::ActorId& id) override
    {
        actors_.erase(id);
        despawned_actors.push_back(id);
    }

    void revive_actor(const gmRules::ActorId& id) override
    {
        auto it = actors_.find(id);
        if (it != actors_.end())
        {
            it->second.alive = true;
            it->second.current_hp = it->second.max_hp > 0 ? it->second.max_hp : 1;
        }
        revived_actors.push_back(id);
    }

    void change_actor_team(const gmRules::ActorId& id,
                           const std::string& team_id) override
    {
        auto it = actors_.find(id);
        if (it != actors_.end())
        {
            it->second.team_id = team_id;
            it->second.faction = team_id;
        }
    }

    // ── Actor resources (Chapter 4) ───────────────────────────────────────────

    void modify_resource(const gmRules::ActorId& id,
                         const std::string& resource_id,
                         int delta) override
    {
        actors_[id].resources[resource_id] += delta;
    }

    void set_resource_max(const gmRules::ActorId& id,
                          const std::string& resource_id,
                          int max_value) override
    {
        resource_max_[id][resource_id] = max_value;
    }

    // ── Equipment (Chapter 4) ─────────────────────────────────────────────────

    void equip_item(const gmRules::ActorId& id,
                    const std::string& item_id) override
    {
        actors_[id].equipped_items.insert(item_id);
    }

    void unequip_item(const gmRules::ActorId& id,
                      const std::string& slot_id) override
    {
        actors_[id].equipped_items.erase(slot_id);
    }

    void add_status_instance(const gmRules::StatusInstance& status) override
    {
        added_statuses.push_back(status);
        actors_[status.owner_actor_id].status_instance_ids.push_back(status.instance_id);
        active_status_instances_[status.instance_id] = status;
    }

    void remove_status_instance(const gmRules::StatusInstanceId& iid) override
    {
        removed_statuses.push_back(iid);
        active_status_instances_.erase(iid);
        for (auto& kv : actors_)
        {
            auto& vec = kv.second.status_instance_ids;
            vec.erase(std::remove(vec.begin(), vec.end(), iid), vec.end());
        }
    }

    bool has_location(const gmRules::LocationId& id) const override
    {
        return locations_.count(id) > 0;
    }

    gmRules::LocationId actor_location(const gmRules::ActorId& id) const override
    {
        auto it = actors_.find(id);
        return it != actors_.end() ? it->second.location_id : "";
    }

    bool are_locations_adjacent(const gmRules::LocationId& a,
                                const gmRules::LocationId& b) const override
    {
        auto it = locations_.find(a);
        if (it == locations_.end()) return false;
        const auto& adj = it->second.adjacent;
        return std::find(adj.begin(), adj.end(), b) != adj.end();
    }

    int distance_between_locations(const gmRules::LocationId& a,
                                   const gmRules::LocationId& b) const override
    {
        if (a == b) return 0;
        if (are_locations_adjacent(a, b)) return 1;
        return -1; // simplified: only 0 or 1 in mock
    }

    bool location_has_tag(const gmRules::LocationId& id,
                          const std::string& tag) const override
    {
        auto it = locations_.find(id);
        if (it == locations_.end()) return false;
        return it->second.tags.count(tag) > 0;
    }

    std::vector<gmRules::ActorId>
    actors_in_location(const gmRules::LocationId& loc) const override
    {
        auto it = locations_.find(loc);
        return it != locations_.end() ? it->second.actors : std::vector<gmRules::ActorId>{};
    }

    // ── Advanced map queries (Chapter 6) ──────────────────────────────────────

    bool is_location_reachable(const gmRules::LocationId& from,
                               const gmRules::LocationId& to) const override
    {
        // Simplified: reachable if connected (adjacent or same)
        if (from == to) return true;
        return are_locations_adjacent(from, to);
    }

    bool has_line_of_sight(const gmRules::LocationId& from,
                           const gmRules::LocationId& to) const override
    {
        // Simplified: always true between adjacent or same locations
        if (from == to) return true;
        return are_locations_adjacent(from, to);
    }

    int move_cost_between(const gmRules::LocationId& from,
                          const gmRules::LocationId& to) const override
    {
        return distance_between_locations(from, to);
    }

    void add_location_tag(const gmRules::LocationId& id,
                          const std::string& tag) override
    {
        locations_[id].tags.insert(tag);
    }

    void remove_location_tag(const gmRules::LocationId& id,
                             const std::string& tag) override
    {
        auto it = locations_.find(id);
        if (it != locations_.end()) it->second.tags.erase(tag);
    }

    void set_location_passable(const gmRules::LocationId& id,
                               bool passable) override
    {
        locations_[id].passable = passable;
    }

    void set_location_owner(const gmRules::LocationId& id,
                            const std::string& owner_id) override
    {
        locations_[id].owner_id = owner_id;
    }

    void create_barrier(const gmRules::LocationId& from,
                        const gmRules::LocationId& to,
                        const std::string& barrier_id) override
    {
        // Remove adjacency to simulate barrier
        auto& adj_from = locations_[from].adjacent;
        adj_from.erase(std::remove(adj_from.begin(), adj_from.end(), to), adj_from.end());
        created_barriers.push_back(barrier_id);
        barrier_locations_[barrier_id] = {from, to};
    }

    void remove_barrier(const std::string& barrier_id) override
    {
        auto it = barrier_locations_.find(barrier_id);
        if (it != barrier_locations_.end())
        {
            // Restore adjacency
            locations_[it->second.first].adjacent.push_back(it->second.second);
            barrier_locations_.erase(it);
        }
        removed_barriers.push_back(barrier_id);
    }

    void spawn_interactable(const gmRules::LocationId& location_id,
                            const std::string& spec_json) override
    {
        interactables_[location_id].push_back(spec_json);
    }

    void despawn_interactable(const std::string& interactable_id) override
    {
        despawned_interactables_.push_back(interactable_id);
    }

    void move_actor_to_location(const gmRules::ActorId& actor_id,
                                const gmRules::LocationId& loc) override
    {
        auto it = actors_.find(actor_id);
        if (it == actors_.end()) return;
        const std::string& old_loc = it->second.location_id;
        auto& old_actors = locations_[old_loc].actors;
        old_actors.erase(std::remove(old_actors.begin(), old_actors.end(), actor_id),
                         old_actors.end());
        it->second.location_id = loc;
        locations_[loc].actors.push_back(actor_id);
    }

    bool has_deck(const gmRules::DeckId& id) const override
    {
        return decks_.count(id) > 0;
    }

    void add_deck(const gmRules::DeckId& id,
                  const std::vector<gmRules::CardId>& cards)
    {
        decks_[id] = cards;
    }

    std::vector<gmRules::CardId> draw_cards(const gmRules::DeckId& id,
                                            int amount) override
    {
        auto it = decks_.find(id);
        if (it == decks_.end()) return {};
        std::vector<gmRules::CardId> drawn;
        for (int i = 0; i < amount && !it->second.empty(); ++i)
        {
            drawn.push_back(it->second.back());
            it->second.pop_back();
        }
        return drawn;
    }

    gmRules::RuleResult move_card_to_zone(const gmRules::DeckId& /*deck_id*/,
                                          const gmRules::CardId& /*card_id*/,
                                          const std::string& /*zone*/) override
    {
        return gmRules::RuleResult::ok();
    }

    // ── Extended deck/dice operations (Chapter 5) ─────────────────────────────

    int deck_zone_count(const gmRules::DeckId& id,
                        const std::string& zone_name) const override
    {
        if (zone_name.empty())
        {
            auto it = decks_.find(id);
            return it != decks_.end() ? static_cast<int>(it->second.size()) : 0;
        }
        auto dit = deck_zones_.find(id);
        if (dit == deck_zones_.end()) return 0;
        auto zit = dit->second.find(zone_name);
        return zit != dit->second.end() ? static_cast<int>(zit->second.size()) : 0;
    }

    bool card_in_zone(const gmRules::DeckId& id,
                      const gmRules::CardId& card_id,
                      const std::string& zone_name) const override
    {
        auto dit = deck_zones_.find(id);
        if (dit == deck_zones_.end()) return false;
        auto zit = dit->second.find(zone_name);
        if (zit == dit->second.end()) return false;
        const auto& cards = zit->second;
        return std::find(cards.begin(), cards.end(), card_id) != cards.end();
    }

    void shuffle_zone(const gmRules::DeckId& /*id*/,
                      const std::string& /*zone_name*/) override
    {
        // No-op in mock; order irrelevant for tests
    }

    std::vector<gmRules::CardId> look_top_cards(const gmRules::DeckId& id,
                                                int count) const override
    {
        auto it = decks_.find(id);
        if (it == decks_.end()) return {};
        const auto& cards = it->second;
        int n = std::min(count, static_cast<int>(cards.size()));
        return std::vector<gmRules::CardId>(cards.end() - n, cards.end());
    }

    std::vector<gmRules::CardId> look_bottom_cards(const gmRules::DeckId& id,
                                                   int count) const override
    {
        auto it = decks_.find(id);
        if (it == decks_.end()) return {};
        const auto& cards = it->second;
        int n = std::min(count, static_cast<int>(cards.size()));
        return std::vector<gmRules::CardId>(cards.begin(), cards.begin() + n);
    }

    gmRules::RuleResult select_specific_card(const gmRules::DeckId& /*id*/,
                                             const gmRules::CardId& /*card_id*/) override
    {
        return gmRules::RuleResult::ok();
    }

    gmRules::RuleResult discard_random_cards(const gmRules::DeckId& id,
                                             const std::string& zone_name,
                                             int count) override
    {
        auto dit = deck_zones_.find(id);
        if (dit == deck_zones_.end()) return gmRules::RuleResult::ok();
        auto zit = dit->second.find(zone_name);
        if (zit == dit->second.end()) return gmRules::RuleResult::ok();
        auto& cards = zit->second;
        int n = std::min(count, static_cast<int>(cards.size()));
        cards.erase(cards.begin(), cards.begin() + n);
        return gmRules::RuleResult::ok();
    }

    gmRules::RuleResult place_card_on_top(const gmRules::DeckId& id,
                                          const gmRules::CardId& card_id) override
    {
        decks_[id].push_back(card_id);
        return gmRules::RuleResult::ok();
    }

    gmRules::RuleResult place_card_on_bottom(const gmRules::DeckId& id,
                                             const gmRules::CardId& card_id) override
    {
        decks_[id].insert(decks_[id].begin(), card_id);
        return gmRules::RuleResult::ok();
    }

    int roll_dice(const std::string& /*dice_expression*/) override
    {
        return 1; // deterministic stub — tests may override via subclass
    }

    void emit_event(const gmRules::RuleEvent& event,
                    const std::string& bus_name = "RuleEvBus") override
    {
        emitted_events.push_back(event);
        emitted_buses.push_back(bus_name);
    }

    gmRules::RuleResult apply_extended_effect(const gmRules::EffectSpec& effect,
                                              const gmRules::TargetRef& target,
                                              const gmRules::ActorId& source_actor_id,
                                              gmRules::RuleEvent* out_event) override
    {
        if (!allow_extended_effects)
        {
            if (out_event != nullptr)
            {
                out_event->type.clear();
                out_event->source_id.clear();
                out_event->target_id.clear();
                out_event->payload_json.clear();
            }
            return gmRules::RuleResult::fail(gmRules::RuleError::UNSUPPORTED_EFFECT,
                                     "MockRuleContext: extended effect not implemented");
        }

        if (out_event != nullptr)
        {
            out_event->type = std::string("gmRules.extended.") + gmRules::effect_type_name(effect.type);
            out_event->source_id = source_actor_id;
            out_event->target_id = target.id;
            out_event->payload_json.clear();
        }
        return gmRules::RuleResult::ok();
    }

    // ── Accessors for test assertions ─────────────────────────────────────────

    int hp(const gmRules::ActorId& id) const
    {
        auto it = actors_.find(id);
        return it != actors_.end() ? it->second.current_hp : -1;
    }

    std::string location(const gmRules::ActorId& id) const
    {
        auto it = actors_.find(id);
        return it != actors_.end() ? it->second.location_id : "";
    }

private:
    std::unordered_map<gmRules::ActorId, ActorData>       actors_;
    std::unordered_map<gmRules::LocationId, LocationData>  locations_;
    std::unordered_map<gmRules::DeckId, std::vector<gmRules::CardId>> decks_;
    std::unordered_map<gmRules::DeckId,
        std::unordered_map<std::string, std::vector<gmRules::CardId>>> deck_zones_;
    std::unordered_map<gmRules::ActorId,
        std::unordered_map<std::string, int>> resource_max_;
    std::unordered_map<gmRules::StatusInstanceId, gmRules::StatusInstance> active_status_instances_;
    std::unordered_map<std::string, std::pair<std::string, std::string>> barrier_locations_;
    std::unordered_map<std::string, std::vector<std::string>> interactables_;
    std::vector<std::string> despawned_interactables_;
};

} // namespace gmRules_test

#endif // GMRULES_TESTS_MOCKRULECONTEXT_HPP
