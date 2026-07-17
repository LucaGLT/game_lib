/**
 * contract — Eldhôm-specific typeId/payload shapes and small REST helpers
 * for endpoints eng_serve exposes beyond the generic session contract
 * (@webgui/session/restClient only knows about /sessions + generic
 * commands, since not every game_lib WebApp has a "missions" concept).
 *
 * Phase 1 scope: just enough to drive the raw-JSON event-log stub page. The
 * full typed contract (~40 eldhom.* event/command typeIds, mirroring
 * GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp) is deferred to Phase 3
 * ("Frontend Functional Parity" — see GAME/Eldhom/WebApp/PLAN.md).
 */

/** Sent by eng_serve's POST /sessions to bootstrap the engine (see session_manager.py). */
export const CMD_START_MISSION = 'eldhom.start_mission'

/** One of the 4 simple actions (MOVE/ATTACK/INTERACT/RECOVER) a hero can take on its turn. */
export const CMD_SIMPLE_ACTION = 'eldhom.simple_action'

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
