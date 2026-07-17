/**
 * DeckTable — the active hero's full 6-zone card table (Mazzo/Mano/Giocate/
 * Memoria/Scarti/Bandite), replacing Phase 4's single-zone `HandPanel`.
 * Port of `GAME/Eldhom/GUI/modules/gm_comp_deck_module.py`'s
 * `GmCompDeckModule` (6-zone drag&drop card table) crossed with
 * `GAME/Eldhom/GUI/app/eldhom_main_window.py`'s `_DeckProxy`, which is the
 * AUTHORITATIVE source of which zone-to-zone moves actually do something
 * for Eldhôm specifically (the desktop widget is generic and nominally
 * supports more moves than this game's engine understands):
 *
 *   CardHand → PlayArea/Memory  : `eldhom.play_card`        (plays the card)
 *   CardHand → DiscardPile      : `eldhom.deck.discard`      (GM override)
 *   MainDeck → CardHand         : `eldhom.deck.draw`         (GM override)
 *   DiscardPile → CardHand      : `eldhom.deck.take_discard` (GM override)
 *   (reshuffle button)          : `eldhom.deck.reshuffle`    (GM override)
 *   Everything else (Memory/PlayArea/BanishZone as a move SOURCE, or as a
 *   destination other than the two above) is a no-op on the real engine —
 *   `_DeckProxy.send_command()`'s final comment: "All other zone moves are
 *   ignored: the engine drives zone changes." This component therefore:
 *     - renders MainDeck as a COUNT-ONLY stack (the engine never reveals
 *       draw-pile card identities — only `deck_count`),
 *     - renders PlayArea from `hero.played_ids` and DiscardPile from
 *       `hero.discard_ids`/`discard_count` (both authoritative wire fields,
 *       refreshed by a fresh `eldhom.state.full` after EVERY command — see
 *       `main.cpp`'s `emit_full_state()` call sites — so no client-side
 *       inference like the desktop's `_hero_deck_state` is needed here),
 *     - renders Memory as an always-empty alternate "play" drop target
 *       (no distinct wire data exists for it in this game), and
 *     - renders BanishZone as always-empty, present only for 1:1 visual
 *       layout parity with the desktop (nothing in Eldhôm's command/event
 *       set ever populates it).
 *
 * Interaction: both native HTML5 drag&drop (`draggable`, `onDragStart`,
 * `onDragOver`, `onDrop`) AND plain click/button alternatives are wired for
 * every action (per the redesign's explicit "both" decision) — drag&drop
 * for the tactile hand→zone moves, small buttons for the GM-override
 * actions (discard/draw/take/reshuffle) that don't have an obvious card to
 * grab (e.g. "draw" starts from a hidden, identity-less deck).
 */
import { useState } from 'react'
import { CARD_TYPE_ICONS, effectSummaryLine } from '../engine/cardIcons'
import type { CardWire, HeroWire } from '../engine/contract'

export interface DeckTableProps {
  heroName: string
  hand: string[]
  hero: HeroWire | undefined
  cards: Record<string, CardWire>
  sequenceActive: boolean
  enabled: boolean
  onPlayCard: (cardId: string) => void
  onDiscardCard: (cardId: string) => void
  onDrawCard: () => void
  onTakeDiscard: () => void
  onReshuffle: () => void
}

/** Drag payload mime type carrying `{cardId, from}` as JSON — namespaced to avoid clashing with other drag sources. */
const DRAG_MIME = 'application/x-eldhom-card'

interface DragPayload {
  cardId: string
  from: 'CardHand' | 'MainDeck' | 'DiscardPile'
}

function isPlayable(card: CardWire | undefined, sequenceActive: boolean): boolean {
  if (!card) {
    return false
  }
  if (card.card_type === 'INSTANT') {
    return false
  }
  if (sequenceActive) {
    return card.card_type === 'SEQ_CONTINUE' || card.card_type === 'SEQ_END'
  }
  return card.card_type === 'SINGLE' || card.card_type === 'SEQ_START'
}

function setDragPayload(event: React.DragEvent, payload: DragPayload): void {
  event.dataTransfer.setData(DRAG_MIME, JSON.stringify(payload))
  event.dataTransfer.effectAllowed = 'move'
}

function readDragPayload(event: React.DragEvent): DragPayload | null {
  const raw = event.dataTransfer.getData(DRAG_MIME)
  if (!raw) {
    return null
  }
  try {
    return JSON.parse(raw) as DragPayload
  } catch {
    return null
  }
}

export function DeckTable({
  heroName,
  hand,
  hero,
  cards,
  sequenceActive,
  enabled,
  onPlayCard,
  onDiscardCard,
  onDrawCard,
  onTakeDiscard,
  onReshuffle,
}: DeckTableProps) {
  const [hoveredCardId, setHoveredCardId] = useState<string | null>(null)
  const [dragOverZone, setDragOverZone] = useState<string | null>(null)

  const deckCount = hero?.deck_count ?? 0
  const discardIds = hero?.discard_ids ?? []
  const discardCount = hero?.discard_count ?? discardIds.length
  const playedIds = hero?.played_ids ?? []
  const topDiscardId = discardIds.length > 0 ? discardIds[discardIds.length - 1] : null

  const detailCard = hoveredCardId ? cards[hoveredCardId] : undefined

  function handlePlayZoneDrop(event: React.DragEvent): void {
    event.preventDefault()
    setDragOverZone(null)
    const payload = readDragPayload(event)
    if (payload && payload.from === 'CardHand' && enabled) {
      onPlayCard(payload.cardId)
    }
  }

  function handleDiscardZoneDrop(event: React.DragEvent): void {
    event.preventDefault()
    setDragOverZone(null)
    const payload = readDragPayload(event)
    if (payload && payload.from === 'CardHand' && enabled) {
      onDiscardCard(payload.cardId)
    }
  }

  function handleHandZoneDrop(event: React.DragEvent): void {
    event.preventDefault()
    setDragOverZone(null)
    const payload = readDragPayload(event)
    if (!payload || !enabled) {
      return
    }
    if (payload.from === 'MainDeck') {
      onDrawCard()
    } else if (payload.from === 'DiscardPile') {
      onTakeDiscard()
    }
  }

  function allowDrop(zone: string) {
    return (event: React.DragEvent) => {
      event.preventDefault()
      setDragOverZone(zone)
    }
  }

  function zoneClass(zone: string, extra = ''): string {
    return `eldhom-deck-table__zone ${extra}${dragOverZone === zone ? ' eldhom-deck-table__zone--drag-over' : ''}`
  }

  return (
    <div className="eldhom-deck-table">
      <div className="eldhom-deck-table__header">
        <p className="eldhom-deck-table__title">
          Tavolo carte — {heroName}
          {sequenceActive ? ' (sequenza in corso)' : ''}
        </p>
        <div className="eldhom-deck-table__detail">
          {detailCard ? (
            <>
              <span className="eldhom-deck-table__detail-name">
                {CARD_TYPE_ICONS[detailCard.card_type]} {detailCard.name}
              </span>
              <span className="eldhom-deck-table__detail-desc">{detailCard.description}</span>
            </>
          ) : (
            <span className="eldhom-deck-table__detail-hint">
              Passa il mouse su una carta per vedere i dettagli. Trascina le carte tra le zone, oppure
              usa i pulsanti.
            </span>
          )}
        </div>
      </div>

      <div className="eldhom-deck-table__zones">
        {/* ── Mazzo (MainDeck) — count-only, drag-source for "Pesca" ─────── */}
        <div className={zoneClass('MainDeck', 'eldhom-deck-table__zone--deck')}>
          <p className="eldhom-deck-table__zone-title">🂠 Mazzo ({deckCount})</p>
          <div
            className="eldhom-deck-table__deck-stack"
            draggable={enabled && deckCount > 0}
            onDragStart={(event) => setDragPayload(event, { cardId: '', from: 'MainDeck' })}
            title="Trascina sulla Mano per pescare una carta"
          >
            {deckCount > 0 ? (
              Array.from({ length: Math.min(deckCount, 4) }).map((_, index) => (
                <span key={index} className="eldhom-deck-table__card-back" />
              ))
            ) : (
              <span className="eldhom-deck-table__zone-empty">vuoto</span>
            )}
          </div>
          <button type="button" className="eldhom-deck-table__btn" disabled={!enabled || deckCount === 0} onClick={onDrawCard}>
            Pesca
          </button>
        </div>

        {/* ── Mano (CardHand) ──────────────────────────────────────────────── */}
        <div
          className={zoneClass('CardHand', 'eldhom-deck-table__zone--hand')}
          onDragOver={allowDrop('CardHand')}
          onDragLeave={() => setDragOverZone(null)}
          onDrop={handleHandZoneDrop}
        >
          <p className="eldhom-deck-table__zone-title">
            🖐 Mano ({hand.length}{hero ? `/${hero.hand_limit}` : ''})
          </p>
          <div className="eldhom-deck-table__cards">
            {hand.length === 0 && <span className="eldhom-deck-table__zone-empty">mano vuota</span>}
            {hand.map((cardId, index) => {
              const card = cards[cardId]
              const playable = enabled && isPlayable(card, sequenceActive)
              const typeIcon = card ? CARD_TYPE_ICONS[card.card_type] : '📄'
              const summary = card
                ? card.effects.map((effect) => effectSummaryLine(effect, card.timeline_cost)).join('  ')
                : ''
              return (
                <div
                  key={`${cardId}-${index}`}
                  className="eldhom-deck-table__card eldhom-deck-table__card--hand"
                  draggable={enabled}
                  onDragStart={(event) => setDragPayload(event, { cardId, from: 'CardHand' })}
                  onMouseEnter={() => setHoveredCardId(cardId)}
                  onFocus={() => setHoveredCardId(cardId)}
                  title={card?.description ?? cardId}
                >
                  <button
                    type="button"
                    className="eldhom-deck-table__card-play"
                    disabled={!playable}
                    onClick={() => onPlayCard(cardId)}
                  >
                    <span className="eldhom-deck-table__card-name">
                      {typeIcon} {card?.name ?? cardId}
                    </span>
                    <span className="eldhom-deck-table__card-summary">{summary}</span>
                  </button>
                  <button
                    type="button"
                    className="eldhom-deck-table__card-discard"
                    disabled={!enabled}
                    title="Scarta (senza giocare)"
                    onClick={() => onDiscardCard(cardId)}
                  >
                    🗑
                  </button>
                </div>
              )
            })}
          </div>
        </div>

        {/* ── Giocate (PlayArea) — read-only, this-turn plays ─────────────── */}
        <div
          className={zoneClass('PlayArea', 'eldhom-deck-table__zone--played')}
          onDragOver={allowDrop('PlayArea')}
          onDragLeave={() => setDragOverZone(null)}
          onDrop={handlePlayZoneDrop}
        >
          <p className="eldhom-deck-table__zone-title">🂡 Giocate ({playedIds.length})</p>
          <div className="eldhom-deck-table__cards">
            {playedIds.length === 0 && <span className="eldhom-deck-table__zone-empty">nessuna carta</span>}
            {playedIds.map((cardId, index) => {
              const card = cards[cardId]
              return (
                <div
                  key={`${cardId}-${index}`}
                  className="eldhom-deck-table__card eldhom-deck-table__card--played"
                  onMouseEnter={() => setHoveredCardId(cardId)}
                  title={card?.description ?? cardId}
                >
                  <span className="eldhom-deck-table__card-name">
                    {card ? CARD_TYPE_ICONS[card.card_type] : '📄'} {card?.name ?? cardId}
                  </span>
                </div>
              )
            })}
          </div>
        </div>

        {/* ── Memoria (Memory) — alternate play target, no distinct engine data ── */}
        <div
          className={zoneClass('Memory', 'eldhom-deck-table__zone--memory')}
          onDragOver={allowDrop('Memory')}
          onDragLeave={() => setDragOverZone(null)}
          onDrop={handlePlayZoneDrop}
          title="Equivalente a Giocate per Eldhôm: rilasciare qui una carta di Mano la gioca comunque"
        >
          <p className="eldhom-deck-table__zone-title">🧠 Memoria (0)</p>
          <span className="eldhom-deck-table__zone-empty">nessuna carta</span>
        </div>

        {/* ── Scarti (DiscardPile) ─────────────────────────────────────────── */}
        <div
          className={zoneClass('DiscardPile', 'eldhom-deck-table__zone--discard')}
          onDragOver={allowDrop('DiscardPile')}
          onDragLeave={() => setDragOverZone(null)}
          onDrop={handleDiscardZoneDrop}
        >
          <p className="eldhom-deck-table__zone-title">🗑 Scarti ({discardCount})</p>
          <div className="eldhom-deck-table__cards">
            {discardIds.length === 0 && <span className="eldhom-deck-table__zone-empty">vuoti</span>}
            {discardIds.map((cardId, index) => {
              const card = cards[cardId]
              const isTop = cardId === topDiscardId && index === discardIds.length - 1
              return (
                <div
                  key={`${cardId}-${index}`}
                  className={`eldhom-deck-table__card eldhom-deck-table__card--discard${isTop ? ' eldhom-deck-table__card--top' : ''}`}
                  draggable={enabled && isTop}
                  onDragStart={
                    isTop ? (event) => setDragPayload(event, { cardId, from: 'DiscardPile' }) : undefined
                  }
                  onMouseEnter={() => setHoveredCardId(cardId)}
                  title={
                    (card?.description ?? cardId) + (isTop ? ' (cima — trascina sulla Mano per riprenderla)' : '')
                  }
                >
                  <span className="eldhom-deck-table__card-name">
                    {card ? CARD_TYPE_ICONS[card.card_type] : '📄'} {card?.name ?? cardId}
                  </span>
                </div>
              )
            })}
          </div>
          <div className="eldhom-deck-table__zone-actions">
            <button type="button" className="eldhom-deck-table__btn" disabled={!enabled || discardIds.length === 0} onClick={onTakeDiscard}>
              Riprendi
            </button>
            <button type="button" className="eldhom-deck-table__btn" disabled={!enabled || discardCount === 0} onClick={onReshuffle}>
              Rimescola
            </button>
          </div>
        </div>

        {/* ── Bandite (BanishZone) — always empty for Eldhôm, layout parity only ── */}
        <div
          className={zoneClass('BanishZone', 'eldhom-deck-table__zone--banish')}
          title="Nessuna carta di Eldhôm viene attualmente bandita: zona presente solo per coerenza di layout"
        >
          <p className="eldhom-deck-table__zone-title">🚫 Bandite (0)</p>
          <span className="eldhom-deck-table__zone-empty">non usata</span>
        </div>
      </div>
    </div>
  )
}
