/**
 * gameState — reducer turning raw `eldhom.*` envelopes into the state
 * consumed by the React components. Mirrors the event handling already
 * implemented (and validated over 94/94 engine tests) by the desktop
 * widgets `GAME/Eldhom/GUI/widgets/board_widget.py` (`EldhomBoardWidget`),
 * `timeline_widget.py` (`TimelineWidget`) and the hand/sequence/reaction
 * wiring in `GAME/Eldhom/GUI/app/eldhom_main_window.py` — see those files
 * for the reference behaviour this ports to TypeScript.
 *
 * Phase 4 adds: card catalog (loaded once via `applyCardCatalog`, not
 * envelope-driven), per-hero hand contents, per-hero sequence-active flag,
 * and the pending TAKE/BLOCK/DODGE reaction window. Formation/instant-window
 * dialog state is Phase 6's responsibility (see GAME/Eldhom/WebApp/PLAN.md).
 */
import type { EngineEnvelope } from '@webgui/session/types'
import {
  EVT_FORMATION_DIALOG_NEEDED,
  EVT_FORMATION_DONE,
  EVT_HAND_UPDATED,
  EVT_INSTANT_WINDOW_CLOSED,
  EVT_INSTANT_WINDOW_OPENED,
  EVT_MISSION_TIME_ADVANCED,
  EVT_MONSTER_DEFEATED,
  EVT_MONSTER_MOVED,
  EVT_PG_MOVED,
  EVT_REACTION_WINDOW_CLOSED,
  EVT_REACTION_WINDOW_OPENED,
  EVT_SEQUENCE_BROKEN,
  EVT_SEQUENCE_ENDED,
  EVT_SEQUENCE_STARTED,
  EVT_STATE_FULL,
  EVT_TURN_NEXT_ACTOR,
  EVT_ZONE_DOOR_OPENED,
  type CardWire,
  type FormationDialogWire,
  type HeroWire,
  type InstantWindowWire,
  type MonsterGroupWire,
  type NextActorWire,
  type ReactionWindowWire,
  type SpecialObjectWire,
  type StateFullWire,
} from './contract'

/** Passage style between two adjacent locations (mirrors `map_scene.py`'s edge_type). */
export type EdgeType = 'FREE' | 'CLOSED_DOOR' | 'LOCKED_DOOR'

/** One location node on the map. */
export interface MapLocation {
  id: string
  name: string
  /** Trailing-digit-stripped id prefix (e.g. "S1" -> "S") — same-zone passages are always FREE. */
  zone: string
  /** Ids of directly-adjacent locations (regular adjacency only, not locked/secret passages). */
  adjacent: string[]
}

/** One rendered passage between two locations. */
export interface MapEdge {
  a: string
  b: string
  type: EdgeType
}

/** One actor token placed on the map (hero or monster instance). */
export interface ActorToken {
  actorId: string
  /** Short display label, e.g. "PG1", "G2", "GE1" — mirrors board_widget.py's labelling scheme. */
  label: string
  isHero: boolean
  /** For monster instances, the owning group's id (a `turn.next_actor` for the group highlights all its instances). */
  groupId: string | null
  location: string
  position: 'FRONTLINE' | 'BACKLINE'
  hp: number
  maxHp: number
  alive: boolean
}

/** One actor row on the activation timeline. */
export interface TimelineActor {
  actorId: string
  name: string
  timeline: number
  isHero: boolean
}

/** Phase 3+4 slice of Eldhôm's game state (map, timeline, hand, sequence, reaction window). */
export interface EldhomState {
  missionId: string
  title: string
  missionTime: number
  isOver: boolean
  locations: MapLocation[]
  edges: MapEdge[]
  tokens: ActorToken[]
  timelineActors: TimelineActor[]
  nextActorId: string
  /** "HERO" | "MONSTER_GROUP" | other — whether the action panel should be shown for nextActorId. */
  nextActorKind: string
  /** Card catalog keyed by card_id, loaded once via `applyCardCatalog` (not envelope-driven). */
  cards: Record<string, CardWire>
  /** Each hero's current hand, as a list of card_id (duplicates allowed). */
  handByHero: Record<string, string[]>
  /** Whether each hero currently has an open card sequence (SEQ_START played, no SEQ_END yet). */
  sequenceActiveByHero: Record<string, boolean>
  /** Set while the engine awaits a TAKE/BLOCK/DODGE choice; null otherwise. */
  pendingReaction: ReactionWindowWire | null
  /** Raw hero wire data keyed by id (HP/resources/hand-count for HeroPanel, Phase 5). */
  heroesById: Record<string, HeroWire>
  /** Raw monster-group wire data (name/timeline/monster_type/instances), for `MonsterGroupPanel`/`ActorDetailModal` (Phase 20) — refreshed only on `state.full`, same cadence as `tokens`/`heroesById`. */
  groups: MonsterGroupWire[]
  /** Set while the engine awaits a mandatory Prima Linea/Retroguardia choice; null otherwise. */
  pendingFormation: FormationDialogWire | null
  /** Set while the engine awaits an instant-card choice (proactive or reactive); null otherwise. */
  pendingInstantWindow: InstantWindowWire | null
  /** Raw special-object wire data (levers/treasure/secret passages), refreshed only on `state.full` — used to check whether INTERACT has anything to do at a location (see App.tsx's `hasAnyAction`). */
  specialObjects: SpecialObjectWire[]
}

export const initialEldhomState: EldhomState = {
  missionId: '',
  title: '',
  missionTime: 0,
  isOver: false,
  locations: [],
  edges: [],
  tokens: [],
  timelineActors: [],
  nextActorId: '',
  nextActorKind: '',
  cards: {},
  handByHero: {},
  sequenceActiveByHero: {},
  pendingReaction: null,
  heroesById: {},
  groups: [],
  pendingFormation: null,
  pendingInstantWindow: null,
  specialObjects: [],
}

/** Loads the card catalog into state (call once after `listCards()` resolves — not envelope-driven). */
export function applyCardCatalog(previous: EldhomState, cards: CardWire[]): EldhomState {
  const byId: Record<string, CardWire> = {}
  for (const card of cards) {
    byId[card.card_id] = card
  }
  return { ...previous, cards: byId }
}

/** Applies one engine envelope to the previous state, returning the next state. */
export function applyEnvelope(previous: EldhomState, envelope: EngineEnvelope): EldhomState {
  switch (envelope.typeId) {
    case EVT_STATE_FULL:
      return applyStateFull(previous, envelope.data as unknown as StateFullWire)
    case EVT_PG_MOVED:
    case EVT_MONSTER_MOVED:
      return moveToken(previous, envelope)
    case EVT_MONSTER_DEFEATED:
      return removeToken(previous, envelope)
    case EVT_ZONE_DOOR_OPENED:
      return openZoneDoor(previous, envelope)
    case EVT_TURN_NEXT_ACTOR:
      return applyNextActor(previous, envelope)
    case EVT_MISSION_TIME_ADVANCED:
      return applyTimeAdvanced(previous, envelope)
    case EVT_HAND_UPDATED:
      return applyHandUpdated(previous, envelope)
    case EVT_SEQUENCE_STARTED:
      return applySequenceActive(previous, envelope, true)
    case EVT_SEQUENCE_ENDED:
    case EVT_SEQUENCE_BROKEN:
      return applySequenceActive(previous, envelope, false)
    case EVT_REACTION_WINDOW_OPENED:
      return { ...previous, pendingReaction: envelope.data as unknown as ReactionWindowWire }
    case EVT_REACTION_WINDOW_CLOSED:
      return { ...previous, pendingReaction: null }
    case EVT_FORMATION_DIALOG_NEEDED:
      return { ...previous, pendingFormation: envelope.data as unknown as FormationDialogWire }
    case EVT_FORMATION_DONE:
      return { ...previous, pendingFormation: null }
    case EVT_INSTANT_WINDOW_OPENED:
      return { ...previous, pendingInstantWindow: envelope.data as unknown as InstantWindowWire }
    case EVT_INSTANT_WINDOW_CLOSED:
      return { ...previous, pendingInstantWindow: null }
    default:
      return previous
  }
}

function pairKey(a: string, b: string): string {
  return a < b ? `${a}|${b}` : `${b}|${a}`
}

/** Strips a trailing numeric suffix from a location id (e.g. "S1" -> "S", "IN" -> "IN"). */
function zoneFromLocationId(locationId: string): string {
  const stripped = locationId.replace(/\d+$/, '')
  return stripped.length > 0 ? stripped : locationId
}

/** Derives a short monster-instance label prefix from its monster_type (mirrors `_monster_prefix`). */
function monsterPrefixFromType(monsterType: string): string {
  const words = monsterType.toLowerCase().split('_').filter((word) => word.length > 0)
  if (words.length === 0) {
    return 'M'
  }
  const first = words[0][0]?.toUpperCase() ?? 'M'
  if (words.includes('elite') || words.includes('boss')) {
    return `${first}E`
  }
  return first
}

/** Best-effort monster label prefix, falling back to id/name heuristics (mirrors `_monster_label_prefix_from_payload`). */
function monsterLabelPrefix(
  monsterType: string,
  instanceId: string,
  groupId: string,
  groupName: string,
): string {
  const prefix = monsterPrefixFromType(monsterType)
  if (prefix !== 'M') {
    return prefix
  }
  const probe = `${instanceId} ${groupId} ${groupName}`.toLowerCase()
  if (probe.includes('elite') || probe.includes('boss')) {
    return 'GE'
  }
  if (probe.includes('guard') || probe.includes('guardiano')) {
    return 'G'
  }
  return 'M'
}

function buildLocations(wire: StateFullWire): MapLocation[] {
  return wire.locations.map((loc) => ({
    id: loc.id,
    name: loc.name,
    zone: zoneFromLocationId(loc.id),
    adjacent: loc.adjacent,
  }))
}

function buildEdges(wire: StateFullWire, locations: MapLocation[]): MapEdge[] {
  const lockedPairs = new Set<string>()
  for (const specialObject of wire.special_objects) {
    for (const [a, b] of specialObject.locked_adjacency) {
      lockedPairs.add(pairKey(a, b))
    }
  }

  const openedDoors = new Set<string>()
  for (const [a, b] of wire.opened_zone_doors) {
    openedDoors.add(pairKey(a, b))
  }

  const zoneById = new Map(locations.map((loc) => [loc.id, loc.zone]))
  const seen = new Set<string>()
  const edges: MapEdge[] = []

  function addEdge(a: string, b: string): void {
    const key = pairKey(a, b)
    if (seen.has(key)) {
      return
    }
    seen.add(key)
    let type: EdgeType
    if (lockedPairs.has(key)) {
      type = 'LOCKED_DOOR'
    } else if (openedDoors.has(key)) {
      type = 'FREE'
    } else if (zoneById.get(a) === zoneById.get(b)) {
      type = 'FREE'
    } else {
      type = 'CLOSED_DOOR'
    }
    edges.push({ a, b, type })
  }

  for (const loc of wire.locations) {
    for (const adjacent of loc.adjacent) {
      addEdge(loc.id, adjacent)
    }
  }
  // Locked passages not present in the regular adjacency lists (e.g. secret
  // passages, mirrors board_widget.py's "not yet adjacent" LOCKED_DOOR pass).
  for (const specialObject of wire.special_objects) {
    for (const [a, b] of specialObject.locked_adjacency) {
      addEdge(a, b)
    }
  }

  return edges
}

function buildTokens(wire: StateFullWire): ActorToken[] {
  const tokens: ActorToken[] = wire.heroes.map((hero, index) => ({
    actorId: hero.id,
    label: `PG${index + 1}`,
    isHero: true,
    groupId: null,
    location: hero.location,
    position: hero.position,
    hp: hero.hp,
    maxHp: hero.max_hp,
    alive: true,
  }))

  const prefixCounters = new Map<string, number>()
  for (const group of wire.groups) {
    for (const instance of group.instances) {
      const prefix = monsterLabelPrefix(group.monster_type, instance.id, group.id, group.name)
      const suffixMatch = /(\d+)$/.exec(instance.id)
      let label: string
      if (suffixMatch) {
        label = `${prefix}${suffixMatch[1]}`
      } else {
        const nextCount = (prefixCounters.get(prefix) ?? 0) + 1
        prefixCounters.set(prefix, nextCount)
        label = `${prefix}${nextCount}`
      }
      tokens.push({
        actorId: instance.id,
        label,
        isHero: false,
        groupId: group.id,
        location: instance.location,
        position: instance.position,
        hp: instance.hp,
        maxHp: instance.max_hp,
        alive: instance.alive,
      })
    }
  }

  return tokens
}

function buildTimelineActors(wire: StateFullWire): TimelineActor[] {
  return [
    ...wire.heroes.map((hero) => ({
      actorId: hero.id,
      name: hero.name,
      timeline: hero.timeline,
      isHero: true,
    })),
    ...wire.groups.map((group) => ({
      actorId: group.id,
      name: group.name,
      timeline: group.timeline,
      isHero: false,
    })),
  ]
}

function applyStateFull(previous: EldhomState, wire: StateFullWire): EldhomState {
  const locations = buildLocations(wire)
  const handByHero: Record<string, string[]> = {}
  const heroesById: Record<string, HeroWire> = {}
  for (const hero of wire.heroes) {
    handByHero[hero.id] = hero.hand
    heroesById[hero.id] = hero
  }
  return {
    ...previous,
    missionId: wire.mission_id,
    title: wire.title,
    missionTime: wire.time,
    isOver: wire.is_over ?? false,
    locations,
    edges: buildEdges(wire, locations),
    tokens: buildTokens(wire),
    heroesById,
    groups: wire.groups,
    timelineActors: buildTimelineActors(wire),
    nextActorId: wire.next_actor?.actor_id ?? '',
    nextActorKind: wire.next_actor?.kind ?? '',
    handByHero,
    specialObjects: wire.special_objects,
  }
}

function moveToken(previous: EldhomState, envelope: EngineEnvelope): EldhomState {
  const actorId = String(envelope.data.actor_id ?? '')
  const destination = String(envelope.data.payload ?? '')
  if (actorId === '' || destination === '') {
    return previous
  }
  return {
    ...previous,
    tokens: previous.tokens.map((token) =>
      token.actorId === actorId ? { ...token, location: destination } : token,
    ),
  }
}

function removeToken(previous: EldhomState, envelope: EngineEnvelope): EldhomState {
  const actorId = String(envelope.data.actor_id ?? '')
  if (actorId === '') {
    return previous
  }
  return {
    ...previous,
    tokens: previous.tokens.filter((token) => token.actorId !== actorId),
  }
}

function openZoneDoor(previous: EldhomState, envelope: EngineEnvelope): EldhomState {
  const payload = envelope.data.payload as { a?: string; b?: string } | undefined
  const a = payload?.a
  const b = payload?.b
  if (a === undefined || b === undefined) {
    return previous
  }
  const key = pairKey(a, b)
  return {
    ...previous,
    edges: previous.edges.map((edge) =>
      pairKey(edge.a, edge.b) === key ? { ...edge, type: 'FREE' } : edge,
    ),
  }
}

function applyNextActor(previous: EldhomState, envelope: EngineEnvelope): EldhomState {
  const data = envelope.data as unknown as NextActorWire
  return {
    ...previous,
    nextActorId: data.actor_id,
    nextActorKind: data.kind,
    timelineActors: previous.timelineActors.map((actor) =>
      actor.actorId === data.actor_id ? { ...actor, timeline: data.actor_timeline } : actor,
    ),
  }
}

function applyTimeAdvanced(previous: EldhomState, envelope: EngineEnvelope): EldhomState {
  const actorId = String(envelope.data.actor_id ?? '')
  const newTimeline = envelope.data.payload
  if (actorId === '' || typeof newTimeline !== 'number') {
    return previous
  }
  return {
    ...previous,
    timelineActors: previous.timelineActors.map((actor) =>
      actor.actorId === actorId ? { ...actor, timeline: newTimeline } : actor,
    ),
  }
}

/** `eldhom.deck.hand_updated`'s `payload` is the hero's FULL current hand (list of card_id). */
function applyHandUpdated(previous: EldhomState, envelope: EngineEnvelope): EldhomState {
  const actorId = String(envelope.data.actor_id ?? '')
  const payload = envelope.data.payload
  if (actorId === '' || !Array.isArray(payload)) {
    return previous
  }
  return {
    ...previous,
    handByHero: { ...previous.handByHero, [actorId]: payload.map((cardId) => String(cardId)) },
  }
}

/** Shared handler for sequence_started (active=true) / ended|broken (active=false). */
function applySequenceActive(
  previous: EldhomState,
  envelope: EngineEnvelope,
  active: boolean,
): EldhomState {
  const actorId = String(envelope.data.actor_id ?? '')
  if (actorId === '') {
    return previous
  }
  return {
    ...previous,
    sequenceActiveByHero: { ...previous.sequenceActiveByHero, [actorId]: active },
  }
}
