#ifndef GMACTOR_EVENTS_ACTOREVENTS_HPP
#define GMACTOR_EVENTS_ACTOREVENTS_HPP

/**
 * @file events/ActorEvents.hpp
 * @brief Struct-only event payload types for actor state changes.
 *
 * These structs carry the data needed by an event handler to respond to
 * actor-related changes.  Publishing is the responsibility of the game engine,
 * typically via `gmDispatch`.  This header has **zero** dependency on
 * `gmDispatch` or any other event bus.
 *
 * ## Event type string constants
 *
 * | Constant                         | Trigger                                |
 * |----------------------------------|----------------------------------------|
 * | `EVT_HP_CHANGED`                 | `current_hp` changed on any actor      |
 * | `EVT_STATUS_ADDED`               | A status was added to an actor         |
 * | `EVT_STATUS_REMOVED`             | A status was removed from an actor     |
 * | `EVT_MOVED_AREA`                 | Actor's `area_id` changed              |
 * | `EVT_POSITION_CHANGED`           | Actor's `area_position` changed        |
 * | `EVT_ITEM_EQUIPPED`              | An item was equipped                   |
 * | `EVT_ITEM_UNEQUIPPED`            | An item was unequipped                 |
 * | `EVT_LIFE_STATE_CHANGED`         | Actor's `life_state` changed           |
 */

#include "gmActor/core/Ids.hpp"
#include "gmActor/core/Enums.hpp"

#include <string>

namespace gmActor {

// ── Event type string constants ───────────────────────────────────────────────

inline constexpr const char* EVT_HP_CHANGED          = "gmActor.actor.hp_changed";
inline constexpr const char* EVT_STATUS_ADDED        = "gmActor.actor.status_added";
inline constexpr const char* EVT_STATUS_REMOVED      = "gmActor.actor.status_removed";
inline constexpr const char* EVT_MOVED_AREA          = "gmActor.actor.moved_area";
inline constexpr const char* EVT_POSITION_CHANGED    = "gmActor.actor.position_changed";
inline constexpr const char* EVT_ITEM_EQUIPPED       = "gmActor.actor.item_equipped";
inline constexpr const char* EVT_ITEM_UNEQUIPPED     = "gmActor.actor.item_unequipped";
inline constexpr const char* EVT_LIFE_STATE_CHANGED  = "gmActor.actor.life_state_changed";

// ── Event payload structs ─────────────────────────────────────────────────────

/** @brief Payload for EVT_HP_CHANGED. */
struct HpChangedEvent {
    ActorId actor_id;   ///< Actor whose HP changed
    int     old_hp;     ///< HP before the change
    int     new_hp;     ///< HP after the change
    int     max_hp;     ///< Maximum HP at the time of the change
    SourceId source_id; ///< Who caused the change (empty if unknown)
};

/** @brief Payload for EVT_STATUS_ADDED. */
struct StatusAddedEvent {
    ActorId  actor_id;  ///< Actor receiving the status
    StatusId status_id; ///< Status that was added
    int      stacks;    ///< Stack count after the addition
    SourceId source_id; ///< Who applied the status
};

/** @brief Payload for EVT_STATUS_REMOVED. */
struct StatusRemovedEvent {
    ActorId  actor_id;  ///< Actor losing the status
    StatusId status_id; ///< Status that was removed
};

/** @brief Payload for EVT_MOVED_AREA. */
struct MovedAreaEvent {
    ActorId actor_id;   ///< Actor that moved
    AreaId  old_area;   ///< Previous area ID (may be empty)
    AreaId  new_area;   ///< New area ID
};

/** @brief Payload for EVT_POSITION_CHANGED. */
struct PositionChangedEvent {
    ActorId      actor_id;    ///< Actor whose position changed
    AreaPosition old_position; ///< Previous position
    AreaPosition new_position; ///< New position
};

/** @brief Payload for EVT_ITEM_EQUIPPED. */
struct ItemEquippedEvent {
    ActorId        actor_id;        ///< Actor equipping the item
    ItemInstanceId item_instance_id; ///< Item instance that was equipped
    EquipmentSlot  slot;            ///< Slot the item was placed in
};

/** @brief Payload for EVT_ITEM_UNEQUIPPED. */
struct ItemUnequippedEvent {
    ActorId        actor_id;        ///< Actor unequipping the item
    ItemInstanceId item_instance_id; ///< Item instance that was unequipped
    EquipmentSlot  slot;            ///< Slot that was cleared
};

/** @brief Payload for EVT_LIFE_STATE_CHANGED. */
struct LifeStateChangedEvent {
    ActorId        actor_id;   ///< Actor whose life state changed
    ActorLifeState old_state;  ///< Previous state
    ActorLifeState new_state;  ///< New state
    SourceId       source_id;  ///< Who caused the change (empty if unknown)
};

} // namespace gmActor

#endif // GMACTOR_EVENTS_ACTOREVENTS_HPP
