/**
 * contract — Eldhôm-specific typeId/payload shapes and small REST helpers
 * for endpoints eng_serve exposes beyond the generic session contract
 * (@webgui/session/restClient only knows about /sessions + generic
 * commands, since not every game_lib WebApp has a "missions" concept).
 *
 * Phase 4 scope: adds typeIds + wire shapes for hand/sequences/actions
 * (cards, reaction window) on top of Phase 3's map/timeline contract. The
 * remaining ~20 eldhom.* event/command typeIds (formation/instant-window
 * dialogs — mirroring GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp) are
 * deferred to Phase 6 ("Dialog Interattive → Modali Web" — see
 * GAME/Eldhom/WebApp/PLAN.md). All field names below are taken verbatim
 * from a direct reading of GAME/Eldhom/CoreEngine/main.cpp's
 * `emit_full_state()`/`forward_engine_event()`/`handle_*()` methods, not
 * assumed from the C++ EventType/CommandType constants alone.
 */
import type { SessionInfo, SessionPreview } from '@webgui/session/types'

/** Sent by eng_serve's POST /sessions to bootstrap the engine (see session_manager.py). */
export const CMD_START_MISSION = 'eldhom.start_mission'

/** One of the 4 simple actions (MOVE/ATTACK/INTERACT/RECOVER) a hero can take on its turn. */
export const CMD_SIMPLE_ACTION = 'eldhom.simple_action'

/** Plays one hand card (destination/target_id supplied only when the card's effects need them). */
export const CMD_PLAY_CARD = 'eldhom.play_card'

/** Ends the active hero's card sequence early (SEQ_START/CONTINUE played, no SEQ_END yet). */
export const CMD_STOP_SEQUENCE = 'eldhom.stop_sequence'

/** Declares a Simple Attack against target_id (opens the defender's reaction window). */
export const CMD_DECLARE_ATTACK = 'eldhom.declare_attack'

/** Sends the defender's chosen reaction ("TAKE"/"BLOCK"/"DODGE") for a pending attack. */
export const CMD_REACT_DEFENSE = 'eldhom.react_defense'

/** Resolves a pending formation dialog with the chosen Retroguardia actor ids. */
export const CMD_RESOLVE_FORMATION = 'eldhom.resolve_formation'

/** Answers a proactive instant-card window (opened after an attack is declared). */
export const CMD_PLAY_INSTANTS = 'eldhom.play_instants'

/** Answers a reactive instant-card window (Assestarsi — an enemy approached). */
export const CMD_PLAY_REACTIVE_INSTANTS = 'eldhom.play_reactive_instants'

/** GM override: draws 1 card for hero_id from MainDeck into CardHand. */
export const CMD_DECK_DRAW = 'eldhom.deck.draw'

/** GM override: discards card_id from hero_id's CardHand directly (no play). */
export const CMD_DECK_DISCARD = 'eldhom.deck.discard'

/** GM override: takes the top card of hero_id's DiscardPile back into CardHand. */
export const CMD_DECK_TAKE_DISCARD = 'eldhom.deck.take_discard'

/** GM override: reshuffles hero_id's DiscardPile back into MainDeck. */
export const CMD_DECK_RESHUFFLE = 'eldhom.deck.reshuffle'

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

/** A hero's hand changed (card played, discarded, drawn, or reshuffled). Payload: full CardId[] hand. */
export const EVT_HAND_UPDATED = 'eldhom.deck.hand_updated'

/** A hero opened a card sequence (played a SEQ_START card). */
export const EVT_SEQUENCE_STARTED = 'eldhom.pg.sequence_started'

/** A hero's card sequence ended normally (SEQ_END played or stop_sequence sent). */
export const EVT_SEQUENCE_ENDED = 'eldhom.pg.sequence_ended'

/** A hero's card sequence was interrupted (e.g. by a reaction window). */
export const EVT_SEQUENCE_BROKEN = 'eldhom.pg.sequence_broken'

/** The engine opened the defender's TAKE/BLOCK/DODGE reaction window (see ReactionWindowWire). */
export const EVT_REACTION_WINDOW_OPENED = 'eldhom.reaction.window_opened'

/** The engine closed the reaction window after resolving it. */
export const EVT_REACTION_WINDOW_CLOSED = 'eldhom.reaction.window_closed'

/** The engine needs a mandatory formation (Prima Linea/Retroguardia) choice (see FormationDialogWire). */
export const EVT_FORMATION_DIALOG_NEEDED = 'eldhom.formation.dialog_needed'

/** The engine finished applying a resolved formation choice. */
export const EVT_FORMATION_DONE = 'eldhom.formation.done'

/** The engine opened a proactive or reactive instant-card window (see InstantWindowWire). */
export const EVT_INSTANT_WINDOW_OPENED = 'eldhom.instant.window_opened'

/** The engine closed the instant-card window after resolving it. */
export const EVT_INSTANT_WINDOW_CLOSED = 'eldhom.instant.window_closed'

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
  faction: string
  location: string
  position: 'FRONTLINE' | 'BACKLINE'
  hp: number
  max_hp: number
  timeline: number
  life_state: number
  hand_limit: number
  hand: string[]
  deck_count: number
  discard_count: number
  discard_ids: string[]
  played_ids: string[]
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
  /**
   * True while the announced actor already completed their one allowed
   * action/card/sequence this turn and must explicitly confirm Fine Turno
   * (a PASS simple_action) before the engine will actually hand the turn to
   * whoever the timeline says is next. See EldhomEngine::has_pending_turn_
   * confirmation(). Absent on older engine builds — treat as false.
   */
  awaiting_confirmation?: boolean
}

/** One card effect entry (`card.effects[]`) — shape varies by `effect_type`, see cards_base.json. */
export interface CardEffectWire {
  effect_type: string
  amount?: number
  target?: string
  value?: string
  attack_type?: 'MELEE' | 'RANGED'
  range?: number
  condition?: string
}

export type CardType = 'SINGLE' | 'INSTANT' | 'SEQ_START' | 'SEQ_CONTINUE' | 'SEQ_END'

/** One entry of GET /cards (raw pass-through of a cards_*.json entry). */
export interface CardWire {
  card_id: string
  name: string
  origin?: string
  card_type: CardType
  timeline_cost: number
  effects: CardEffectWire[]
  description?: string
  reaction_trigger?: string
  requires_frontline?: boolean
  condition?: string
}

/** Full payload (`data`) of an `eldhom.reaction.window_opened` envelope (sent at the data root, not under `payload`). */
export interface ReactionWindowWire {
  attacker_id: string
  defender_id: string
  group_id?: string
  incoming_damage: number
  reactions: string[]
}

/** One entry of a formation dialog's `actors` array. */
export interface FormationActorWire {
  actor_id: string
  name: string
  in_backline: boolean
}

/** Full payload (`data`) of an `eldhom.formation.dialog_needed` envelope (sent at the data root). */
export interface FormationDialogWire {
  location_id: string
  faction_id: string
  /** "scompaginamento" | "overflow" | "disrupt" — see FormationModal's SOURCE_LABELS. */
  source: string
  actors: FormationActorWire[]
}

/** One playable instant option (`eldhom.instant.window_opened`'s `options` array). */
export interface InstantOptionWire {
  actor_id: string
  card_id: string
  card_name: string
}

/** Full payload (`data`) of an `eldhom.instant.window_opened` envelope (sent at the data root). */
export interface InstantWindowWire {
  /** The triggering typeId — "eldhom.pg.enemy_approach" means this is the REACTIVE variant
   *  (resolve with CMD_PLAY_REACTIVE_INSTANTS), anything else is the proactive variant
   *  (resolve with CMD_PLAY_INSTANTS). Both variants reuse the same event/modal. */
  trigger: string
  options: InstantOptionWire[]
}

/** One entry of GET /missions. */
export interface MissionSummary {
  mission_id: string
  title: string
  description: string
  pg_roster: PgRosterEntry[]
}

/** One playable PG/hero of a mission's roster (`MissionSummary.pg_roster`) — used to render the "pick your PG/hero" screen (see `HeroSelectModal`). */
export interface PgRosterEntry {
  hero_id: string
  display_name: string
  class_name: string
}

/** Response of `GET /sessions/by-code/{code}` (Shared Multiplayer: preview before joining). */
export interface EldhomSessionPreview extends SessionPreview {
  mission_id: string | null
}

/** Eldhôm's `SessionInfo` (create/join/list/get) adds `mission_id` on top of the generic shared shape. */
export interface EldhomSessionInfo extends SessionInfo {
  mission_id: string | null
}

/** Lists the card catalog (GET /cards — Eldhôm-specific, ex module-level _CARD_CATALOG). */
export async function listCards(token: string): Promise<CardWire[]> {
  const response = await fetch('/cards', { headers: { Authorization: `Bearer ${token}` } })
  if (!response.ok) {
    throw new Error(`listCards failed: HTTP ${response.status}`)
  }
  return (await response.json()) as CardWire[]
}

/** Lists available missions, incl. each one's `pg_roster` (GET /missions — Eldhôm-specific, ex MissionSelectDialog). */
export async function listMissions(token: string): Promise<MissionSummary[]> {
  const response = await fetch('/missions', { headers: { Authorization: `Bearer ${token}` } })
  if (!response.ok) {
    throw new Error(`listMissions failed: HTTP ${response.status}`)
  }
  return (await response.json()) as MissionSummary[]
}
