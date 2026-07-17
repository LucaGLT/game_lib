/**
 * contract — Eldhôm-specific typeId/payload shapes and small REST helpers
 * for endpoints eng_serve exposes beyond the generic session contract
 * (@webgui/session/restClient only knows about /sessions + generic
 * commands, since not every game_lib WebApp has a "missions" concept).
 *
 * Phase 3 scope: typeIds + wire shapes needed by the map (`EldhomMap`) and
 * timeline (`TimelineTrack`) components. The remaining ~30 eldhom.*
 * event/command typeIds (hand/sequences/actions/formation/instants —
 * mirroring GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp) are deferred to
 * Phase 4+ ("Frontend Functional Parity" — see GAME/Eldhom/WebApp/PLAN.md).
 * All field names below are taken verbatim from a direct reading of
 * GAME/Eldhom/CoreEngine/main.cpp's `emit_full_state()`/`forward_engine_event()`,
 * not assumed from the C++ EventType constants alone.
 */

/** Sent by eng_serve's POST /sessions to bootstrap the engine (see session_manager.py). */
export const CMD_START_MISSION = 'eldhom.start_mission'

/** One of the 4 simple actions (MOVE/ATTACK/INTERACT/RECOVER) a hero can take on its turn. */
export const CMD_SIMPLE_ACTION = 'eldhom.simple_action'

/** Full state snapshot, sent on mission start / reconnect (see StateFullWire). */
export const EVT_STATE_FULL = 'eldhom.state.full'

/** A hero moved (MOVE simple action or a move card). Payload: destination LocationId. */
export const EVT_PG_MOVED = 'eldhom.pg.moved'

/** A monster instance moved (behavior card resolution). Payload: destination LocationId. */
export const EVT_MONSTER_MOVED = 'eldhom.monster.moved'

/** A monster instance was defeated and its token should be removed from the map. */
export const EVT_MONSTER_DEFEATED = 'eldhom.monster.defeated'

/** A PG crossed a CLOSED_DOOR zone boundary, opening it permanently. Payload: {a, b}. */
export const EVT_ZONE_DOOR_OPENED = 'eldhom.zone_door.opened'

/** Sent after each turn ends, before the next begins (see NextActorWire). */
export const EVT_TURN_NEXT_ACTOR = 'eldhom.turn.next_actor'

/** A PG's timeline position advanced after paying an action's ⌛ cost. */
export const EVT_MISSION_TIME_ADVANCED = 'eldhom.mission.time_advanced'

/** One entry of `eldhom.state.full`'s `locations` array. */
export interface LocationWire {
  id: string
  name: string
  adjacent: string[]
}

/** One entry of `eldhom.state.full`'s `heroes` array. */
export interface HeroWire {
  id: string
  name: string
  location: string
  position: 'FRONTLINE' | 'BACKLINE'
  hp: number
  max_hp: number
  timeline: number
}

/** One entry of a monster group's `instances` array. */
export interface MonsterInstanceWire {
  id: string
  location: string
  position: 'FRONTLINE' | 'BACKLINE'
  hp: number
  max_hp: number
  alive: boolean
}

/** One entry of `eldhom.state.full`'s `groups` array. */
export interface MonsterGroupWire {
  id: string
  name: string
  timeline: number
  location: string
  monster_type: string
  instances: MonsterInstanceWire[]
}

/** One entry of `eldhom.state.full`'s `special_objects` array. */
export interface SpecialObjectWire {
  object_id: string
  type: string
  location_id: string
  locked_adjacency: Array<[string, string]>
}

/** Full payload (`data`) of an `eldhom.state.full` envelope. */
export interface StateFullWire {
  mission_id: string
  title: string
  time: number
  locations: LocationWire[]
  heroes: HeroWire[]
  groups: MonsterGroupWire[]
  special_objects: SpecialObjectWire[]
  opened_zone_doors: Array<[string, string]>
  next_actor: { actor_id: string; kind: string }
  is_over?: boolean
}

/** Full payload (`data`) of an `eldhom.turn.next_actor` envelope. */
export interface NextActorWire {
  actor_id: string
  actor_name: string
  actor_timeline: number
  kind: string
  mission_time: number
}

/** One entry of GET /missions. */
export interface MissionSummary {
  mission_id: string
  title: string
  description: string
}

/** Lists available missions (GET /missions — Eldhôm-specific, ex MissionSelectDialog). */
export async function listMissions(): Promise<MissionSummary[]> {
  const response = await fetch('/missions')
  if (!response.ok) {
    throw new Error(`listMissions failed: HTTP ${response.status}`)
  }
  return (await response.json()) as MissionSummary[]
}
