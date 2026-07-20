import { useEffect, useRef, useState, type CSSProperties } from 'react'
import { useAuth } from '@webgui/session/AuthProvider'
import { LoginForm } from '@webgui/components/LoginForm'
import { JoinSessionForm } from '@webgui/components/JoinSessionForm'
import {
  createSession,
  listSessions,
  closeSession,
  sendCommand,
  joinSession,
  getSession,
  previewSessionByCode,
} from '@webgui/session/restClient'
import { connectSessionEvents } from '@webgui/session/wsClient'
import { EnvelopeRouter } from '@webgui/session/EnvelopeRouter'
import { useGmGuiModule } from '@webgui/modules/useGmGuiModule'
import { DEFAULT_THEME_ID, THEMES, getTheme, themeToCssVars, type ThemeId } from '@webgui/theme/themes'
import { ErrorBar } from '@webgui/components/ErrorBar'
import { EventLog, type EventLogEntry } from '@webgui/components/EventLog'
import { ThemeSelect } from '@webgui/components/ThemeSelect'
import '@webgui/styles.css'
import {
  CMD_DECK_DISCARD,
  CMD_DECK_DRAW,
  CMD_DECK_RESHUFFLE,
  CMD_DECK_TAKE_DISCARD,
  CMD_DECLARE_ATTACK,
  CMD_PLAY_CARD,
  CMD_PLAY_INSTANTS,
  CMD_PLAY_REACTIVE_INSTANTS,
  CMD_REACT_DEFENSE,
  CMD_RESOLVE_FORMATION,
  CMD_SIMPLE_ACTION,
  CMD_STOP_SEQUENCE,
  listCards,
  listMissions,
  type EldhomSessionInfo,
  type EldhomSessionPreview,
  type InstantOptionWire,
  type MissionSummary,
} from './engine/contract'
import { hasEffect, isPlayable } from './engine/cardIcons'
import { applyCardCatalog, applyEnvelope, initialEldhomState, type EldhomState } from './engine/gameState'
import { formatEvent, resetLogTimeTracking } from './engine/logFormat'
import { ActionPanel, type TargetingMode } from './components/ActionPanel'
import { ActorDetailModal, type ActorDetailSubject } from './components/ActorDetailModal'
import { AreaInfoPanel } from './components/AreaInfoPanel'
import { DeckTable } from './components/DeckTable'
import { EldhomMap } from './components/EldhomMap'
import { FormationModal } from './components/FormationModal'
import { TimelineTrack } from './components/TimelineTrack'
import { HeroSelectModal } from './components/HeroSelectModal'
import { InstantWindowModal } from './components/InstantWindowModal'
import { MainMenuModal } from './components/MainMenuModal'
import { MissionDetailsModal } from './components/MissionDetailsModal'
import { MissionSelectModal } from './components/MissionSelectModal'
import './App.css'

const THEME_STORAGE_KEY = 'eldhom-webapp-theme'

function loadStoredTheme(): ThemeId {
  const stored = window.localStorage.getItem(THEME_STORAGE_KEY)
  return THEMES.find((theme) => theme.id === stored)?.id ?? DEFAULT_THEME_ID
}

/**
 * "Sotto-tema" — Eldhôm-only background-art variants for EVERY theme
 * (Phase 21 introduced this for `dark_moon` only, as "Sfondo Luna"; this
 * turn generalises it to all 5 themes per explicit user request —
 * "mi è piaciuta l'idea di avere dei Sotto Temi [...] fa lo stesso per
 * TUTTI gli altri temi"). Deliberately NOT added to the shared
 * `ThemeId`/`THEMES` registry (`webLib/WebGUI_Lib/src/theme/themes.ts`):
 * that file mirrors the desktop PySide6 app's fixed 5 themes 1:1 and is
 * shared with other WebApps (Tris) — these are purely a `.app`/
 * `.eldhom-map` `background-image` swap layered on top of the existing
 * theme's color tokens, scoped to this component via a `data-theme-variant`
 * attribute (see `App.css`). Each theme keeps its OWN independent id space
 * and variant COUNT (Scroll has 4, the other 4 themes have 5) — exactly the
 * list the user asked for, no invented extra entries.
 */
type ThemeVariant = { id: string; displayName: string }

const THEME_VARIANTS: Record<ThemeId, ReadonlyArray<ThemeVariant>> = {
  scroll: [
    { id: 'ancient_library', displayName: 'Ancient Library' },
    { id: 'arcane_manuscript', displayName: 'Arcane Manuscript' },
    { id: 'alchemical_scriptorium', displayName: 'Alchemical Scriptorium' },
    { id: 'nautical_chart', displayName: 'Nautical Chart' },
  ],
  stone: [
    { id: 'elder_stone', displayName: 'Elder Stone' },
    { id: 'frost_runes', displayName: 'Frost Runes' },
    { id: 'obsidian_codex', displayName: 'Obsidian Codex' },
    { id: 'primeval_dolmen', displayName: 'Primeval Dolmen' },
    { id: 'reliquary', displayName: 'Reliquary' },
  ],
  dark_moon: [
    { id: 'moon_01', displayName: 'Crepuscolo' },
    { id: 'moon_02', displayName: 'Aurora' },
    { id: 'moon_03', displayName: 'Nebulosa' },
    { id: 'moon_04', displayName: 'Nebbia' },
    { id: 'moon_05', displayName: 'Profondo' },
  ],
  blood: [
    { id: 'sacrificial_altar', displayName: 'Sacrificial Altar' },
    { id: 'crimson_earth', displayName: 'Crimson Earth' },
    { id: 'eclipse_runestone', displayName: 'Eclipse Runestone' },
    { id: 'iron_blood', displayName: 'Iron & Blood' },
    { id: 'nocturnal_blood', displayName: 'Nocturnal Blood' },
  ],
  techno: [
    { id: 'motherboard', displayName: 'Motherboard' },
    { id: 'holographic', displayName: 'Holographic' },
    { id: 'clockwork_brass', displayName: 'Clockwork Brass' },
    { id: 'distopia', displayName: 'Distopia' },
    { id: 'quantum_monolith', displayName: 'Quantum Monolith' },
  ],
}

const THEME_VARIANT_STORAGE_KEY = 'eldhom-webapp-theme-variant'

/** Reads the per-theme remembered variant choice from localStorage, defaulting any theme with no (or an invalid) stored choice to its own first variant. */
function loadStoredThemeVariants(): Record<ThemeId, string> {
  const defaults = Object.fromEntries(
    THEMES.map((theme) => [theme.id, THEME_VARIANTS[theme.id][0].id]),
  ) as Record<ThemeId, string>
  const raw = window.localStorage.getItem(THEME_VARIANT_STORAGE_KEY)
  if (raw === null) {
    return defaults
  }
  try {
    const stored = JSON.parse(raw) as Partial<Record<ThemeId, string>>
    for (const theme of THEMES) {
      const candidate = stored[theme.id]
      if (candidate !== undefined && THEME_VARIANTS[theme.id].some((variant) => variant.id === candidate)) {
        defaults[theme.id] = candidate
      }
    }
  } catch {
    // Malformed storage: fall back to defaults built above.
  }
  return defaults
}

/** What a location/token click (or card drop) on the map currently resolves to (or null if nothing is armed). */
type Targeting =
  | { kind: 'simple-move' }
  | { kind: 'simple-attack' }
  | { kind: 'card-attack'; cardId: string; destination?: string }
  | null

/** Which synthetic card's "extended card" popup (`ActorDetailModal`) is open, by id — resolved against live state on every render (see `resolveDetailSubject`) rather than storing a snapshot, so the popup never shows stale HP/state while open. */
type DetailTarget = { kind: 'hero'; id: string } | { kind: 'monsterGroup'; id: string } | null

/** Looks up the live hero/group data for `target` in `state`. Returns null if the target is unset or no longer exists (e.g. a monster group fully defeated since the popup was opened) — `App` treats a null result as "close the popup". */
function resolveDetailSubject(target: DetailTarget, state: EldhomState): ActorDetailSubject | null {
  if (target === null) {
    return null
  }
  if (target.kind === 'hero') {
    const hero = state.heroesById[target.id]
    return hero ? { kind: 'hero', hero } : null
  }
  const group = state.groups.find((candidate) => candidate.id === target.id)
  return group ? { kind: 'monsterGroup', group } : null
}

/**
 * Main Eldhôm WebApp page. Ties together mission selection, session
 * lifecycle, and all game-state-driven panels: `ActionPanel` (4 simple
 * actions + inline TAKE/BLOCK/DODGE reaction window), `EldhomMap`/
 * `TimelineTrack` (each tile now also carries the per-actor HP/alive-count/
 * location info that used to live in a separate hero/monster-group card
 * row — removed entirely, Migliorie Grafiche 01), `DeckTable` (the full
 * 6-zone card table — see that component's docstring for exactly which
 * zone-to-zone moves are real for Eldhôm vs display-only), the narrative
 * event log (coloured, see `logFormat.ts`) plus a collapsed raw-JSON debug
 * log, and the Phase 6 modals (mission select / formation / instant window).
 *
 * Before any session exists, `showMissionSelect` picks between the two
 * pre-game screens (Phase 19): `MainMenuModal` (landing screen, 4 entries —
 * only "Gioca una missione" is wired up) and, once that is chosen,
 * `MissionSelectModal` (dismissible back to the main menu).
 *
 * Clicking any `TimelineTrack` tile sets `detailTarget` (an id, not a data
 * snapshot — see `resolveDetailSubject`) which opens `ActorDetailModal`
 * (Phase 20) with that actor's/group's "extended card". Resolving against
 * live state on every render (rather than storing the clicked object) means
 * the popup keeps showing fresh HP/state while open and auto-closes if the
 * underlying actor disappears (e.g. a fully-defeated monster group).
 *
 * A "Sotto-tema" picker (generalised from Phase 21's dark_moon-only
 * "Sfondo Luna") lets the user cycle through several alternative
 * background-art variants for WHICHEVER theme is active
 * (`themeVariants`/`THEME_VARIANTS`) — see `App.css`'s
 * `[data-theme-variant]` rules. This is deliberately NOT part of the shared
 * `ThemeId` registry (see the comment above `THEME_VARIANTS`).
 *
 * Point-and-click targeting for the 4 SIMPLE actions (move destination /
 * attack target) is armed by `ActionPanel` and resolved by this
 * component's `handleLocationClick`/`handleTokenClick`, which own the
 * shared `targeting` state consumed by `EldhomMap` — same split as the
 * desktop's `move_armed`/`attack_armed` signals resolved by
 * `EldhomMainWindow`.
 *
 * Playing a HAND CARD, however, is drag&drop-only (clicking never plays a
 * card — see `DeckTable`'s hand card render, which has no `onClick` on its
 * play face): dropping a card onto `EldhomMap` resolves it directly via
 * `handleCardDropOnLocation` (MOVE-effect cards, dropped on a location) /
 * `handleCardDropOnToken` (DAMAGE-effect cards, dropped on a token);
 * dropping onto `DeckTable`'s Giocate/Memoria zones plays cards that need
 * neither. The two paths share `targeting`'s `'card-attack'` state only for
 * the rare move-then-attack chained card (e.g. "Passo e Lama"): the
 * location drop arms it with a destination already set, completed by a
 * following token click/drop.
 *
 * Generic building blocks (theme, session REST/WS client, EnvelopeRouter,
 * ErrorBar/EventLog/ThemeSelect) come from `webLib/WebGUI_Lib` (`@webgui/*`)
 * — consumed from day one here (unlike Tris, which only adopted it in a
 * later refactor). Everything imported from `./engine`/`./components` stays
 * Eldhôm-specific.
 */
function App() {
  const auth = useAuth()
  const [missions, setMissions] = useState<MissionSummary[]>([])
  const [sessions, setSessions] = useState<EldhomSessionInfo[]>([])
  const [showNewMissionFlow, setShowNewMissionFlow] = useState(false)
  const [showMissionSelect, setShowMissionSelect] = useState(false)
  const [missionForHeroPick, setMissionForHeroPick] = useState<MissionSummary | null>(null)
  const [joinPreview, setJoinPreview] = useState<{ joinCode: string; preview: EldhomSessionPreview } | null>(
    null,
  )
  const [sessionId, setSessionId] = useState<string | null>(null)
  const [activeSession, setActiveSession] = useState<EldhomSessionInfo | null>(null)
  const [router, setRouter] = useState<EnvelopeRouter | null>(null)
  const [logEntries, setLogEntries] = useState<string[]>([])
  const [narrativeLog, setNarrativeLog] = useState<EventLogEntry[]>([])
  const [eldhomState, setEldhomState] = useState(initialEldhomState)
  const [targeting, setTargeting] = useState<Targeting>(null)
  const [selectedLocationId, setSelectedLocationId] = useState<string | null>(null)
  const [detailTarget, setDetailTarget] = useState<DetailTarget>(null)
  const [errorMessage, setErrorMessage] = useState<string | null>(null)
  const [showMissionDetails, setShowMissionDetails] = useState(false)
  const [themeId, setThemeId] = useState<ThemeId>(loadStoredTheme)
  const [themeVariants, setThemeVariants] = useState<Record<ThemeId, string>>(loadStoredThemeVariants)
  const disconnectRef = useRef<(() => void) | null>(null)
  const authToken = auth.session?.token ?? null

  useEffect(() => {
    return () => disconnectRef.current?.()
  }, [])

  useEffect(() => {
    window.localStorage.setItem(THEME_STORAGE_KEY, themeId)
  }, [themeId])

  useEffect(() => {
    window.localStorage.setItem(THEME_VARIANT_STORAGE_KEY, JSON.stringify(themeVariants))
  }, [themeVariants])

  useEffect(() => {
    if (authToken === null) {
      return
    }
    listMissions(authToken)
      .then((found) => setMissions(found))
      .catch((caught) => setErrorMessage(String(caught)))
    listSessions(authToken)
      .then((found) => setSessions(found as EldhomSessionInfo[]))
      .catch((caught) => setErrorMessage(String(caught)))
  }, [authToken])

  useEffect(() => {
    if (authToken === null) {
      return
    }
    listCards(authToken)
      .then((found) => setEldhomState((previous) => applyCardCatalog(previous, found)))
      .catch((caught) => setErrorMessage(String(caught)))
  }, [authToken])

  // Wildcard subscription: every envelope feeds the raw-JSON debug log
  // (Phase 1), the Eldhôm-specific narrative log (Phase 5, `logFormat.ts`),
  // and the structured game state (Phase 3+4) — three independent consumers
  // of the same `EnvelopeRouter`, same pattern as Tris' reducer +
  // ActorStatusBadges module.
  useGmGuiModule(router, { subscribedTypeIds: ['*'] }, (envelope) => {
    setLogEntries((previous) => [...previous, JSON.stringify(envelope)])
    setNarrativeLog((previous) => [...previous, ...formatEvent(envelope)])
    setEldhomState((previous) => applyEnvelope(previous, envelope))
  })

  const myHeroId = activeSession?.your_role ?? null
  const activeHeroId = eldhomState.nextActorKind === 'HERO' ? eldhomState.nextActorId : ''
  const activeHeroName =
    eldhomState.timelineActors.find((actor) => actor.actorId === activeHeroId)?.name ?? activeHeroId
  const activeHeroHand = eldhomState.handByHero[activeHeroId] ?? []
  const activeHeroSequenceActive = eldhomState.sequenceActiveByHero[activeHeroId] ?? false
  const pendingReaction = eldhomState.pendingReaction
  const pendingReactionView =
    pendingReaction === null
      ? null
      : {
          defenderName:
            eldhomState.tokens.find((token) => token.actorId === pendingReaction.defender_id)?.label ??
            pendingReaction.defender_id,
          incomingDamage: pendingReaction.incoming_damage,
          reactions: pendingReaction.reactions,
        }
  // Shared Multiplayer turn gate: this participant may act if it is THEIR
  // hero's turn, or if THEIR hero is the one facing the pending reaction —
  // mirrors Tris' `activeMark !== myRole` board-disable check. The server
  // enforces this regardless (see session_manager._HERO_OWNED_COMMAND_FIELDS),
  // this is purely UX feedback so the controls look disabled instead of
  // silently failing.
  const isMyTurn = myHeroId !== null && activeHeroId === myHeroId
  const isMyPendingReaction = myHeroId !== null && pendingReaction?.defender_id === myHeroId
  const canAct = isMyTurn || isMyPendingReaction

  // Player's own hero display name (properly capitalized, e.g. "Velyr") —
  // used for the persistent role banner AND the turn-status message, which
  // must always address THIS participant, never whichever hero currently
  // acts (that can be a teammate while this participant waits their turn).
  // 3-tier fallback: the authoritative wire name (`HeroWire.name`, only
  // populated once the mission has actually started — see
  // `applyStateFull`), else the chosen mission's roster `display_name`
  // (already available right after session creation, while
  // "in attesa di altri giocatori"), else the raw (lowercase) hero_id as a
  // last resort if neither has loaded yet.
  const myRosterDisplayName =
    myHeroId !== null
      ? missions
          .find((mission) => mission.mission_id === activeSession?.mission_id)
          ?.pg_roster.find((entry) => entry.hero_id === myHeroId)?.display_name
      : undefined
  const myHeroName =
    myHeroId !== null ? (eldhomState.heroesById[myHeroId]?.name ?? myRosterDisplayName ?? myHeroId) : ''

  // "Nessuna azione disponibile" (explicit user request): the active hero
  // must ALWAYS be able to press Fine Turno, but when NONE of the 4 base
  // actions nor any hand card would do anything meaningful, those (not Fine
  // Turno) should look disabled instead of inviting a pointless click.
  // MOVE is deliberately NOT part of this check — a hero can always at
  // least attempt to move (near-universal fallback outside true dead ends)
  // and the engine already returns a clear error if a chosen destination
  // turns out to be invalid, same as it always has.
  const activeHero = eldhomState.heroesById[activeHeroId]
  const canAttackNow =
    activeHero !== undefined &&
    eldhomState.tokens.some((token) => !token.isHero && token.alive && token.location === activeHero.location)
  const canInteractNow =
    activeHero !== undefined &&
    eldhomState.specialObjects.some((object) => object.location_id === activeHero.location)
  const canRecoverNow = activeHero !== undefined && activeHero.hp < activeHero.max_hp
  const hasPlayableCardNow = activeHeroHand.some((cardId) =>
    isPlayable(eldhomState.cards[cardId], activeHeroSequenceActive),
  )
  const hasAnyAction = canAttackNow || canInteractNow || canRecoverNow || hasPlayableCardNow

  // actor_id -> display name/label, used by InstantWindowModal (heroes get
  // their full name, monster instances get their short map-token label).
  const actorNames: Record<string, string> = {}
  for (const token of eldhomState.tokens) {
    actorNames[token.actorId] = token.isHero
      ? (eldhomState.heroesById[token.actorId]?.name ?? token.label)
      : token.label
  }

  // Clears any armed targeting when the turn changes or a reaction window
  // opens/closes, so a stale "move armed" state never survives past the
  // hero it was armed for.
  const hasPendingReaction = pendingReaction !== null
  useEffect(() => {
    setTargeting(null)
  }, [activeHeroId, hasPendingReaction])

  /** Wires a freshly created/joined/resumed session's router + WS + reset local state — shared by every entry point below. */
  function connectToSession(token: string, session: EldhomSessionInfo): void {
    disconnectRef.current?.()
    const nextRouter = new EnvelopeRouter()
    setRouter(nextRouter)
    setSessionId(session.session_id)
    setActiveSession(session)
    setLogEntries([])
    setNarrativeLog([])
    resetLogTimeTracking()
    setEldhomState((previous) => applyCardCatalog(initialEldhomState, Object.values(previous.cards)))
    setTargeting(null)
    setSelectedLocationId(null)
    setDetailTarget(null)
    setErrorMessage(null)
    disconnectRef.current = connectSessionEvents(token, session.session_id, (envelope) => {
      nextRouter.dispatch(envelope)
    })
  }

  function friendlyErrorMessage(caught: unknown): string {
    const message = caught instanceof Error ? caught.message : String(caught)
    if (message.includes('429')) {
      return 'Hai raggiunto il numero massimo di missioni contemporanee: chiudine una per continuare.'
    }
    if (message.includes('previewSessionByCode') && message.includes('404')) {
      return 'Codice missione non valido.'
    }
    if (message.includes('joinSession') && message.includes('409')) {
      return 'Quel PG è già stato scelto da un altro giocatore.'
    }
    return message
  }

  /** Step 1 of "Nuova missione": mission chosen (MissionSelectModal) — now pick a PG (HeroSelectModal). */
  function handleMissionSelected(missionId: string): void {
    const mission = missions.find((candidate) => candidate.mission_id === missionId) ?? null
    setShowMissionSelect(false)
    setMissionForHeroPick(mission)
  }

  /** Step 2: PG chosen for a NEW session — actually creates it and seats the caller. */
  async function handleHeroChosenForNewSession(heroId: string): Promise<void> {
    if (authToken === null || missionForHeroPick === null) {
      return
    }
    try {
      const session = await createSession(authToken, { mission_id: missionForHeroPick.mission_id, hero_id: heroId })
      const typedSession = session as EldhomSessionInfo
      setSessions((previous) => [...previous.filter((s) => s.session_id !== typedSession.session_id), typedSession])
      setMissionForHeroPick(null)
      setShowNewMissionFlow(false)
      connectToSession(authToken, typedSession)
    } catch (caught) {
      setErrorMessage(friendlyErrorMessage(caught))
    }
  }

  /** "Entra con codice": previews the session's roster/free seats BEFORE joining (JoinSessionForm.onSubmit). */
  async function handleJoinSession(joinCode: string): Promise<void> {
    if (authToken === null) {
      return
    }
    try {
      const preview = await previewSessionByCode<EldhomSessionPreview>(authToken, joinCode)
      setJoinPreview({ joinCode, preview })
    } catch (caught) {
      const message = friendlyErrorMessage(caught)
      setErrorMessage(message)
      throw new Error(message)
    }
  }

  /** PG chosen (among the remaining free seats) for a mission joined via code. */
  async function handleHeroChosenForJoin(heroId: string): Promise<void> {
    if (authToken === null || joinPreview === null) {
      return
    }
    try {
      const session = await joinSession(authToken, joinPreview.joinCode, { hero_id: heroId })
      const typedSession = session as EldhomSessionInfo
      setSessions((previous) => [...previous.filter((s) => s.session_id !== typedSession.session_id), typedSession])
      setJoinPreview(null)
      connectToSession(authToken, typedSession)
    } catch (caught) {
      setErrorMessage(friendlyErrorMessage(caught))
    }
  }

  function handleResumeSession(session: EldhomSessionInfo): void {
    if (authToken === null) {
      return
    }
    connectToSession(authToken, session)
  }

  async function handleCloseSession(targetSessionId: string): Promise<void> {
    if (authToken === null) {
      return
    }
    try {
      await closeSession(authToken, targetSessionId)
    } catch (caught) {
      setErrorMessage(String(caught))
      return
    }
    setSessions((previous) => previous.filter((s) => s.session_id !== targetSessionId))
    if (sessionId === targetSessionId) {
      disconnectRef.current?.()
      setSessionId(null)
      setActiveSession(null)
    }
  }

  function handleLogout(): void {
    disconnectRef.current?.()
    setSessionId(null)
    setActiveSession(null)
    setSessions([])
    auth.logout()
  }

  // Opponent-join polling: while a shared session still has a free seat,
  // check every 3s so the creator's screen updates automatically once a
  // second (or third+) user joins — the C++ engine has no native "someone
  // joined" event (joining is a pure eng_serve/session-registry concept).
  const needsMorePlayers = activeSession !== null && Object.values(activeSession.roles).some((h) => h === null)
  useEffect(() => {
    if (authToken === null || sessionId === null || !needsMorePlayers) {
      return undefined
    }
    const intervalId = window.setInterval(() => {
      getSession(authToken, sessionId)
        .then((session) => setActiveSession(session as EldhomSessionInfo))
        .catch(() => {
          /* transient poll failure — next tick retries */
        })
    }, 3000)
    return () => window.clearInterval(intervalId)
  }, [authToken, sessionId, needsMorePlayers])

  async function sendEldhomCommand(typeId: string, data: Record<string, unknown>): Promise<void> {
    if (sessionId === null || authToken === null) {
      setErrorMessage('Nessuna sessione attiva: avvia prima una missione.')
      return
    }
    try {
      await sendCommand(authToken, sessionId, typeId, data)
    } catch (caught) {
      setErrorMessage(String(caught))
    }
  }

  function handleArmMove(): void {
    setTargeting((previous) => (previous?.kind === 'simple-move' ? null : { kind: 'simple-move' }))
  }

  function handleArmAttack(): void {
    setTargeting((previous) =>
      previous?.kind === 'simple-attack' || previous?.kind === 'card-attack'
        ? null
        : { kind: 'simple-attack' },
    )
  }

  async function handleInteract(): Promise<void> {
    await sendEldhomCommand(CMD_SIMPLE_ACTION, { hero_id: activeHeroId, action_type: 'INTERACT' })
  }

  async function handleRecover(): Promise<void> {
    await sendEldhomCommand(CMD_SIMPLE_ACTION, {
      hero_id: activeHeroId,
      action_type: 'RECOVER',
      discard_ids: [],
    })
  }

  /** "Fine Turno": explicit user-confirmed pass, even with no productive action left (see PLAN.md). */
  async function handleEndTurn(): Promise<void> {
    await sendEldhomCommand(CMD_SIMPLE_ACTION, { hero_id: activeHeroId, action_type: 'PASS' })
  }

  async function handleStopSequence(): Promise<void> {
    await sendEldhomCommand(CMD_STOP_SEQUENCE, { hero_id: activeHeroId })
  }

  async function handleDrawCard(): Promise<void> {
    await sendEldhomCommand(CMD_DECK_DRAW, { hero_id: activeHeroId })
  }

  async function handleDiscardCard(cardId: string): Promise<void> {
    await sendEldhomCommand(CMD_DECK_DISCARD, { hero_id: activeHeroId, card_id: cardId })
  }

  async function handleTakeDiscard(): Promise<void> {
    await sendEldhomCommand(CMD_DECK_TAKE_DISCARD, { hero_id: activeHeroId })
  }

  async function handleReshuffleDiscard(): Promise<void> {
    await sendEldhomCommand(CMD_DECK_RESHUFFLE, { hero_id: activeHeroId })
  }

  /** Bandite is a placeholder button for now: the dedicated banned-cards management page doesn't exist yet. */
  function handleOpenBanish(): void {
    setErrorMessage('🚫 Gestione carte bandite: pagina dedicata non ancora disponibile.')
  }

  async function handleReactionChosen(reaction: string): Promise<void> {
    if (pendingReaction === null) {
      return
    }
    await sendEldhomCommand(CMD_REACT_DEFENSE, { defender_id: pendingReaction.defender_id, reaction })
  }

  async function handleResolveFormation(backlineActorIds: string[]): Promise<void> {
    const pendingFormation = eldhomState.pendingFormation
    if (pendingFormation === null) {
      return
    }
    // Optimistic clear: the engine immediately re-sends a fresh
    // eldhom.formation.dialog_needed if another formation is still pending
    // after this one resolves (see handle_resolve_formation in main.cpp).
    setEldhomState((previous) => ({ ...previous, pendingFormation: null }))
    await sendEldhomCommand(CMD_RESOLVE_FORMATION, {
      faction_id: pendingFormation.faction_id,
      location_id: pendingFormation.location_id,
      backline: backlineActorIds,
    })
  }

  async function handlePlayInstants(selected: InstantOptionWire[]): Promise<void> {
    const pendingInstantWindow = eldhomState.pendingInstantWindow
    if (pendingInstantWindow === null) {
      return
    }
    setEldhomState((previous) => ({ ...previous, pendingInstantWindow: null }))
    const payload = {
      selected: selected.map((option) => ({ actor_id: option.actor_id, card_id: option.card_id })),
    }
    const isReactive = pendingInstantWindow.trigger === 'eldhom.pg.enemy_approach'
    await sendEldhomCommand(isReactive ? CMD_PLAY_REACTIVE_INSTANTS : CMD_PLAY_INSTANTS, payload)
  }

  // Only reachable for cards needing NEITHER a destination nor a target —
  // `DeckTable`'s Giocate/Memoria drop zones already filter those out before
  // calling this, so the guard below is defensive. Cards needing one are
  // only playable via `handleCardDropOnLocation`/`handleCardDropOnToken`.
  function handlePlayCard(cardId: string): void {
    const card = eldhomState.cards[cardId]
    if (!card || hasEffect(card, 'MOVE', 'MOVE_TOWARD_PG') || hasEffect(card, 'DAMAGE', 'DEAL_DAMAGE')) {
      return
    }
    void sendEldhomCommand(CMD_PLAY_CARD, { hero_id: activeHeroId, card_id: cardId })
  }

  /** Drop of a hand card onto a Map location — only MOVE-effect cards resolve here. */
  async function handleCardDropOnLocation(cardId: string, locationId: string): Promise<void> {
    const card = eldhomState.cards[cardId]
    if (!card || !hasEffect(card, 'MOVE', 'MOVE_TOWARD_PG')) {
      return
    }
    // Some sequence cards (e.g. "Passo e Lama") move THEN attack — arm
    // attack targeting with the destination already set instead of sending
    // immediately, completed by dropping/clicking the same card on a token
    // (mirrors the desktop's _try_move()/"has_damage" branch).
    if (hasEffect(card, 'DAMAGE', 'DEAL_DAMAGE')) {
      setTargeting({ kind: 'card-attack', cardId, destination: locationId })
      return
    }
    await sendEldhomCommand(CMD_PLAY_CARD, {
      hero_id: activeHeroId,
      card_id: cardId,
      destination: locationId,
    })
  }

  /** Drop of a hand card onto a Map token (actor) — only DAMAGE-effect cards resolve here. */
  async function handleCardDropOnToken(cardId: string, actorId: string): Promise<void> {
    const card = eldhomState.cards[cardId]
    if (!card || !hasEffect(card, 'DAMAGE', 'DEAL_DAMAGE')) {
      return
    }
    // A card that also moves must be dropped on a location FIRST (arms
    // 'card-attack' with a destination above) — dropping it straight on a
    // token without that step is not a valid shortcut.
    const pending = targeting?.kind === 'card-attack' && targeting.cardId === cardId ? targeting : null
    if (hasEffect(card, 'MOVE', 'MOVE_TOWARD_PG') && !pending) {
      return
    }
    if (eldhomState.tokens.find((token) => token.actorId === actorId)?.isHero) {
      setErrorMessage('⚠ Seleziona un nemico valido da attaccare')
      return
    }
    setTargeting(null)
    const data: Record<string, unknown> = { hero_id: activeHeroId, card_id: cardId, target_id: actorId }
    if (pending?.destination !== undefined) {
      data.destination = pending.destination
    }
    await sendEldhomCommand(CMD_PLAY_CARD, data)
  }

  async function handleLocationClick(locationId: string): Promise<void> {
    if (targeting === null) {
      setSelectedLocationId(locationId)
      return
    }
    if (targeting.kind === 'simple-move') {
      setTargeting(null)
      await sendEldhomCommand(CMD_SIMPLE_ACTION, {
        hero_id: activeHeroId,
        action_type: 'MOVE',
        destination: locationId,
      })
    }
  }

  async function handleTokenClick(actorId: string): Promise<void> {
    if (targeting === null) {
      const token = eldhomState.tokens.find((candidate) => candidate.actorId === actorId)
      if (token) {
        setSelectedLocationId(token.location)
      }
      return
    }
    // Mirrors the desktop's _actor_is_enemy() guard: reject a self/ally click
    // during attack targeting with a friendly message instead of relying on
    // the engine's auto-target fallback (documented in _pre_play_card_hook)
    // to silently redirect it to a valid enemy.
    if (eldhomState.tokens.find((token) => token.actorId === actorId)?.isHero) {
      setErrorMessage('⚠ Seleziona un nemico valido da attaccare')
      return
    }
    if (targeting.kind === 'simple-attack') {
      setTargeting(null)
      await sendEldhomCommand(CMD_DECLARE_ATTACK, { hero_id: activeHeroId, target_id: actorId })
      return
    }
    if (targeting.kind === 'card-attack') {
      const { cardId, destination } = targeting
      setTargeting(null)
      const data: Record<string, unknown> = { hero_id: activeHeroId, card_id: cardId, target_id: actorId }
      if (destination !== undefined) {
        data.destination = destination
      }
      await sendEldhomCommand(CMD_PLAY_CARD, data)
    }
  }

  const theme = getTheme(themeId)
  const currentThemeVariant = themeVariants[themeId] ?? THEME_VARIANTS[themeId][0].id
  function setCurrentThemeVariant(variantId: string): void {
    setThemeVariants((previous) => ({ ...previous, [themeId]: variantId }))
  }

  if (auth.isRestoring) {
    return (
      <div
        className="app"
        data-theme={theme.id}
        data-theme-variant={currentThemeVariant}
        style={themeToCssVars(theme) as CSSProperties}
      >
        <p>Verifica sessione in corso…</p>
      </div>
    )
  }

  if (!auth.isAuthenticated) {
    return (
      <div
        className="app"
        data-theme={theme.id}
        data-theme-variant={currentThemeVariant}
        style={themeToCssVars(theme) as CSSProperties}
      >
        <LoginForm
          title="Le Pergamene di Eldhôm — Accedi"
          onSubmit={(username, password) => auth.login(username, password)}
        />
      </div>
    )
  }

  const targetingMode: TargetingMode =
    targeting?.kind === 'simple-move'
      ? 'move'
      : targeting?.kind === 'simple-attack' || targeting?.kind === 'card-attack'
        ? 'attack'
        : null

  const selectedLocation =
    selectedLocationId === null
      ? null
      : (eldhomState.locations.find((location) => location.id === selectedLocationId) ?? null)
  const selectedLocationAdjacentNames =
    selectedLocation?.adjacent.map(
      (id) => eldhomState.locations.find((location) => location.id === id)?.name ?? id,
    ) ?? []
  const selectedLocationActors =
    selectedLocationId === null
      ? []
      : eldhomState.tokens
          .filter((token) => token.location === selectedLocationId)
          .map((token) => ({
            actorId: token.actorId,
            label: token.label,
            isHero: token.isHero,
            hp: token.hp,
            maxHp: token.maxHp,
            position: token.position,
          }))

  const detailSubject = resolveDetailSubject(detailTarget, eldhomState)

  return (
    <div
      className="app"
      data-theme={theme.id}
      data-theme-variant={currentThemeVariant}
      style={themeToCssVars(theme) as CSSProperties}
    >
      <header className="app-header">
        <h1>Eldhôm — WebApp</h1>
        <ThemeSelect themeId={themeId} onThemeChange={setThemeId} />
        <label className="gmgui-field">
          Sotto-tema
          <select
            value={currentThemeVariant}
            onChange={(event) => setCurrentThemeVariant(event.target.value)}
          >
            {THEME_VARIANTS[theme.id].map((variant) => (
              <option key={variant.id} value={variant.id}>
                {variant.displayName}
              </option>
            ))}
          </select>
        </label>
        <div className="app-header__account">
          <span>{auth.session?.username}</span>
          {sessionId !== null && (
            <button type="button" onClick={() => void handleCloseSession(sessionId)}>
              Chiudi missione
            </button>
          )}
          <button type="button" onClick={handleLogout}>
            Esci
          </button>
        </div>
      </header>

      {sessionId === null && !showNewMissionFlow && (
        <div className="gmgui-session-picker">
          <h2>Missioni attive</h2>
          {sessions.length === 0 && (
            <p>Nessuna missione attiva. Premi «Nuova missione» per iniziare, oppure entra in una missione con un codice.</p>
          )}
          {sessions.map((session) => (
            <div key={session.session_id} className="gmgui-session-picker__row">
              <span>
                Missione {session.session_id.slice(0, 8)} ·{' '}
                <span className="gmgui-session-picker__code">{session.join_code}</span>
                {' · '}
                {Object.entries(session.roles)
                  .map(([hero, username]) => `${hero}: ${username ?? 'in attesa'}`)
                  .join(' · ')}
              </span>
              <div className="gmgui-session-picker__row-actions">
                <button type="button" onClick={() => handleResumeSession(session)}>
                  Riprendi
                </button>
                <button type="button" onClick={() => void handleCloseSession(session.session_id)}>
                  Chiudi
                </button>
              </div>
            </div>
          ))}
          <div className="eldhom-modal__actions">
            <button type="button" onClick={() => setShowNewMissionFlow(true)}>
              ⚔ Nuova missione
            </button>
          </div>
          <JoinSessionForm onSubmit={handleJoinSession} title="Entra in una missione" />
        </div>
      )}

      {sessionId === null && showNewMissionFlow && !showMissionSelect && missionForHeroPick === null && (
        <MainMenuModal onPlayMission={() => setShowMissionSelect(true)} />
      )}

      {sessionId === null && showMissionSelect && missionForHeroPick === null && (
        <MissionSelectModal
          missions={missions}
          onSelect={handleMissionSelected}
          onDismiss={() => setShowMissionSelect(false)}
        />
      )}

      {sessionId === null && missionForHeroPick !== null && (
        <HeroSelectModal
          heroes={missionForHeroPick.pg_roster}
          title={`Scegli il tuo PG — ${missionForHeroPick.title}`}
          onSelect={(heroId) => void handleHeroChosenForNewSession(heroId)}
          onDismiss={() => setMissionForHeroPick(null)}
        />
      )}

      {joinPreview !== null && (
        <HeroSelectModal
          heroes={
            missions.find((m) => m.mission_id === joinPreview.preview.mission_id)?.pg_roster ??
            Object.keys(joinPreview.preview.roles).map((heroId) => ({
              hero_id: heroId,
              display_name: heroId,
              class_name: '',
            }))
          }
          takenBy={joinPreview.preview.roles}
          title="Scegli il tuo PG (rimasti)"
          onSelect={(heroId) => void handleHeroChosenForJoin(heroId)}
          onDismiss={() => setJoinPreview(null)}
        />
      )}

      {sessionId !== null && myHeroId !== null && (
        <p className="eldhom-role-banner">
          Sei <strong>{myHeroName}</strong>
          {needsMorePlayers && activeSession !== null && (
            <>
              {' — in attesa di altri giocatori. Condividi il codice: '}
              <span className="gmgui-session-picker__code">{activeSession.join_code}</span>
            </>
          )}
        </p>
      )}

      {sessionId !== null && myHeroId !== null && (
        <p className="eldhom-turn-status">
          {activeHeroId !== '' && canAct
            ? `Tocca a Te, ${myHeroName}`
            : myHeroName !== ''
              ? `${myHeroName}, attendi che gli altri facciano le loro Azioni`
              : 'In attesa…'}
        </p>
      )}

      {eldhomState.pendingFormation && (
        <FormationModal
          locationId={eldhomState.pendingFormation.location_id}
          factionId={eldhomState.pendingFormation.faction_id}
          source={eldhomState.pendingFormation.source}
          actors={eldhomState.pendingFormation.actors}
          onConfirm={(backlineIds) => void handleResolveFormation(backlineIds)}
        />
      )}

      {eldhomState.pendingInstantWindow && (
        <InstantWindowModal
          options={eldhomState.pendingInstantWindow.options}
          actorNames={actorNames}
          onConfirm={(selected) => void handlePlayInstants(selected)}
        />
      )}

      {detailSubject && (
        <ActorDetailModal
          subject={detailSubject}
          tokens={eldhomState.tokens}
          onDismiss={() => setDetailTarget(null)}
        />
      )}

      {eldhomState.title !== '' && (
        <button
          type="button"
          className="eldhom-mission-title"
          onClick={() => setShowMissionDetails(true)}
          title="Clicca per i dettagli della missione"
        >
          SessionCode : {activeSession?.join_code ?? '—'} -- Missione : {eldhomState.title}
          {eldhomState.isOver ? ' — Missione conclusa' : ''}
        </button>
      )}

      {showMissionDetails && (
        <MissionDetailsModal
          title={eldhomState.title}
          joinCode={activeSession?.join_code ?? '—'}
          description={missions.find((mission) => mission.mission_id === eldhomState.missionId)?.description ?? ''}
          onDismiss={() => setShowMissionDetails(false)}
        />
      )}

      <TimelineTrack
        actors={eldhomState.timelineActors}
        activeActorId={eldhomState.nextActorId}
        onActorClick={(id, isHero) =>
          setDetailTarget(isHero ? { kind: 'hero', id } : { kind: 'monsterGroup', id })
        }
      />

      <div className="eldhom-board-row">
        <EldhomMap
          locations={eldhomState.locations}
          edges={eldhomState.edges}
          tokens={eldhomState.tokens}
          specialObjects={eldhomState.specialObjects}
          activeActorId={eldhomState.nextActorId}
          onLocationClick={(id) => void handleLocationClick(id)}
          onTokenClick={(id) => void handleTokenClick(id)}
          onCardDropOnLocation={(cardId, id) => void handleCardDropOnLocation(cardId, id)}
          onCardDropOnToken={(cardId, id) => void handleCardDropOnToken(cardId, id)}
        />

        <AreaInfoPanel
          locationName={selectedLocation?.name ?? null}
          adjacentNames={selectedLocationAdjacentNames}
          actors={selectedLocationActors}
          onActorClick={(id) => void handleTokenClick(id)}
        />
      </div>

      <ActionPanel
        activeHeroName={activeHeroName}
        enabled={activeHeroId !== '' && canAct}
        sequenceActive={activeHeroSequenceActive}
        hasAnyAction={hasAnyAction}
        awaitingConfirmation={eldhomState.turnAwaitingConfirmation}
        targetingMode={targetingMode}
        pendingReaction={pendingReactionView}
        onArmMove={handleArmMove}
        onArmAttack={handleArmAttack}
        onInteract={() => void handleInteract()}
        onRecover={() => void handleRecover()}
        onEndTurn={() => void handleEndTurn()}
        onStopSequence={() => void handleStopSequence()}
        onReactionChosen={(reaction) => void handleReactionChosen(reaction)}
      />

      <DeckTable
        heroName={activeHeroName}
        hand={activeHeroHand}
        hero={eldhomState.heroesById[activeHeroId]}
        cards={eldhomState.cards}
        sequenceActive={activeHeroSequenceActive}
        enabled={
          activeHeroId !== '' &&
          pendingReactionView === null &&
          isMyTurn &&
          !eldhomState.turnAwaitingConfirmation
        }
        onPlayCard={handlePlayCard}
        onDiscardCard={(cardId) => void handleDiscardCard(cardId)}
        onDrawCard={() => void handleDrawCard()}
        onTakeDiscard={() => void handleTakeDiscard()}
        onReshuffle={() => void handleReshuffleDiscard()}
        onOpenBanish={handleOpenBanish}
      />

      <EventLog entries={narrativeLog} ariaLabel="Log narrativo" />
      <details className="eldhom-debug-log">
        <summary>🛠 Log eventi grezzi (JSON) — solo debug</summary>
        <EventLog entries={logEntries} ariaLabel="Log eventi grezzi (JSON)" />
      </details>
      <ErrorBar message={errorMessage} />
    </div>
  )
}

export default App
