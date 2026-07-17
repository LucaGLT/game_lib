import { useEffect, useRef, useState, type CSSProperties } from 'react'
import { createSession, sendCommand } from '@webgui/session/restClient'
import { connectSessionEvents } from '@webgui/session/wsClient'
import { EnvelopeRouter } from '@webgui/session/EnvelopeRouter'
import { useGmGuiModule } from '@webgui/modules/useGmGuiModule'
import { DEFAULT_THEME_ID, THEMES, getTheme, themeToCssVars, type ThemeId } from '@webgui/theme/themes'
import { ErrorBar } from '@webgui/components/ErrorBar'
import { EventLog } from '@webgui/components/EventLog'
import { ThemeSelect } from '@webgui/components/ThemeSelect'
import '@webgui/styles.css'
import {
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
  type InstantOptionWire,
  type MissionSummary,
} from './engine/contract'
import { hasEffect } from './engine/cardIcons'
import { applyCardCatalog, applyEnvelope, initialEldhomState } from './engine/gameState'
import { formatEvent, resetLogTimeTracking } from './engine/logFormat'
import { ActionPanel, type TargetingMode } from './components/ActionPanel'
import { AreaInfoPanel } from './components/AreaInfoPanel'
import { EldhomMap } from './components/EldhomMap'
import { FormationModal } from './components/FormationModal'
import { HandPanel } from './components/HandPanel'
import { HeroPanel } from './components/HeroPanel'
import { InstantWindowModal } from './components/InstantWindowModal'
import { MissionSelectModal } from './components/MissionSelectModal'
import { TimelineTrack } from './components/TimelineTrack'
import './App.css'

const THEME_STORAGE_KEY = 'eldhom-webapp-theme'

function loadStoredTheme(): ThemeId {
  const stored = window.localStorage.getItem(THEME_STORAGE_KEY)
  return THEMES.find((theme) => theme.id === stored)?.id ?? DEFAULT_THEME_ID
}

/** What a location/token click on the map currently resolves to (or null if nothing is armed). */
type Targeting =
  | { kind: 'simple-move' }
  | { kind: 'simple-attack' }
  | { kind: 'card-move'; cardId: string }
  | { kind: 'card-attack'; cardId: string; destination?: string }
  | null

/**
 * Phase 4 "Mano, Sequenze & Azioni" page: adds the real `HandPanel`
 * (playable cards, type/effect icons) and `ActionPanel` (4 simple actions +
 * inline TAKE/BLOCK/DODGE reaction window) on top of Phase 3's
 * `EldhomMap`/`TimelineTrack`, plus the Phase 1 raw-JSON event log (kept
 * for debugging). Formation/instant-window dialogs are Phase 6's scope.
 *
 * Point-and-click targeting (move destination / attack target) is armed by
 * `ActionPanel`/`HandPanel` and resolved by this component's
 * `handleLocationClick`/`handleTokenClick`, which own the shared
 * `targeting` state consumed by `EldhomMap` — same split as the desktop's
 * `move_armed`/`attack_armed` signals resolved by `EldhomMainWindow`.
 *
 * Generic building blocks (theme, session REST/WS client, EnvelopeRouter,
 * ErrorBar/EventLog/ThemeSelect) come from `webLib/WebGUI_Lib` (`@webgui/*`)
 * — consumed from day one here (unlike Tris, which only adopted it in a
 * later refactor). Everything imported from `./engine`/`./components` stays
 * Eldhôm-specific.
 */
function App() {
  const [missions, setMissions] = useState<MissionSummary[]>([])
  const [sessionId, setSessionId] = useState<string | null>(null)
  const [router, setRouter] = useState<EnvelopeRouter | null>(null)
  const [logEntries, setLogEntries] = useState<string[]>([])
  const [narrativeLog, setNarrativeLog] = useState<string[]>([])
  const [eldhomState, setEldhomState] = useState(initialEldhomState)
  const [targeting, setTargeting] = useState<Targeting>(null)
  const [selectedLocationId, setSelectedLocationId] = useState<string | null>(null)
  const [errorMessage, setErrorMessage] = useState<string | null>(null)
  const [themeId, setThemeId] = useState<ThemeId>(loadStoredTheme)
  const disconnectRef = useRef<(() => void) | null>(null)

  useEffect(() => {
    return () => disconnectRef.current?.()
  }, [])

  useEffect(() => {
    window.localStorage.setItem(THEME_STORAGE_KEY, themeId)
  }, [themeId])

  useEffect(() => {
    listMissions()
      .then((found) => setMissions(found))
      .catch((caught) => setErrorMessage(String(caught)))
  }, [])

  useEffect(() => {
    listCards()
      .then((found) => setEldhomState((previous) => applyCardCatalog(previous, found)))
      .catch((caught) => setErrorMessage(String(caught)))
  }, [])

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

  async function handleStartMission(missionId: string): Promise<void> {
    try {
      const session = await createSession({ mission_id: missionId })
      disconnectRef.current?.()
      const nextRouter = new EnvelopeRouter()
      setRouter(nextRouter)
      setSessionId(session.session_id)
      setLogEntries([])
      setNarrativeLog([])
      resetLogTimeTracking()
      setEldhomState((previous) => applyCardCatalog(initialEldhomState, Object.values(previous.cards)))
      setTargeting(null)
      setSelectedLocationId(null)
      setErrorMessage(null)
      disconnectRef.current = connectSessionEvents(session.session_id, (envelope) => {
        nextRouter.dispatch(envelope)
      })
    } catch (caught) {
      setErrorMessage(String(caught))
    }
  }

  async function sendEldhomCommand(typeId: string, data: Record<string, unknown>): Promise<void> {
    if (sessionId === null) {
      setErrorMessage('Nessuna sessione attiva: avvia prima una missione.')
      return
    }
    try {
      await sendCommand(sessionId, typeId, data)
    } catch (caught) {
      setErrorMessage(String(caught))
    }
  }

  function handleArmMove(): void {
    setTargeting((previous) =>
      previous?.kind === 'simple-move' || previous?.kind === 'card-move'
        ? null
        : { kind: 'simple-move' },
    )
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

  async function handleStopSequence(): Promise<void> {
    await sendEldhomCommand(CMD_STOP_SEQUENCE, { hero_id: activeHeroId })
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

  function handlePlayCard(cardId: string): void {
    const card = eldhomState.cards[cardId]
    if (!card) {
      return
    }
    if (hasEffect(card, 'MOVE', 'MOVE_TOWARD_PG')) {
      setTargeting({ kind: 'card-move', cardId })
      return
    }
    if (hasEffect(card, 'DAMAGE', 'DEAL_DAMAGE')) {
      setTargeting({ kind: 'card-attack', cardId })
      return
    }
    void sendEldhomCommand(CMD_PLAY_CARD, { hero_id: activeHeroId, card_id: cardId })
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
      return
    }
    if (targeting.kind === 'card-move') {
      const { cardId } = targeting
      const card = eldhomState.cards[cardId]
      // Some sequence cards (e.g. "Passo e Lama") move THEN attack — chain
      // into attack targeting instead of sending immediately, mirroring
      // the desktop's _try_move()/"has_damage" branch.
      if (card && hasEffect(card, 'DAMAGE', 'DEAL_DAMAGE')) {
        setTargeting({ kind: 'card-attack', cardId, destination: locationId })
        return
      }
      setTargeting(null)
      await sendEldhomCommand(CMD_PLAY_CARD, {
        hero_id: activeHeroId,
        card_id: cardId,
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
  const targetingMode: TargetingMode =
    targeting?.kind === 'simple-move' || targeting?.kind === 'card-move'
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

  return (
    <div className="app" style={themeToCssVars(theme) as CSSProperties}>
      <header className="app-header">
        <h1>Eldhôm — WebApp</h1>
        <ThemeSelect themeId={themeId} onThemeChange={setThemeId} />
      </header>

      {sessionId === null && (
        <MissionSelectModal missions={missions} onSelect={(id) => void handleStartMission(id)} />
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

      {eldhomState.title !== '' && (
        <p className="eldhom-mission-title">
          {eldhomState.title} — ⏳ {eldhomState.missionTime}
          {eldhomState.isOver ? ' — Missione conclusa' : ''}
        </p>
      )}

      <ActionPanel
        heroName={activeHeroName}
        enabled={activeHeroId !== ''}
        sequenceActive={activeHeroSequenceActive}
        targetingMode={targetingMode}
        pendingReaction={pendingReactionView}
        onArmMove={handleArmMove}
        onArmAttack={handleArmAttack}
        onInteract={() => void handleInteract()}
        onRecover={() => void handleRecover()}
        onStopSequence={() => void handleStopSequence()}
        onReactionChosen={(reaction) => void handleReactionChosen(reaction)}
      />

      <TimelineTrack actors={eldhomState.timelineActors} activeActorId={eldhomState.nextActorId} />

      <div className="eldhom-hero-row">
        {Object.values(eldhomState.heroesById).map((hero) => (
          <HeroPanel key={hero.id} hero={hero} isActive={hero.id === eldhomState.nextActorId} />
        ))}
      </div>

      <div className="eldhom-board-row">
        <EldhomMap
          locations={eldhomState.locations}
          edges={eldhomState.edges}
          tokens={eldhomState.tokens}
          activeActorId={eldhomState.nextActorId}
          onLocationClick={(id) => void handleLocationClick(id)}
          onTokenClick={(id) => void handleTokenClick(id)}
        />

        <AreaInfoPanel
          locationName={selectedLocation?.name ?? null}
          adjacentNames={selectedLocationAdjacentNames}
          actors={selectedLocationActors}
          onActorClick={(id) => void handleTokenClick(id)}
        />
      </div>

      <HandPanel
        heroName={activeHeroName}
        cardIds={activeHeroHand}
        cards={eldhomState.cards}
        sequenceActive={activeHeroSequenceActive}
        enabled={activeHeroId !== '' && pendingReactionView === null}
        onPlayCard={handlePlayCard}
      />

      <EventLog entries={narrativeLog} ariaLabel="Log narrativo" />
      <EventLog entries={logEntries} ariaLabel="Log eventi grezzi (JSON)" />
      <ErrorBar message={errorMessage} />
    </div>
  )
}

export default App
