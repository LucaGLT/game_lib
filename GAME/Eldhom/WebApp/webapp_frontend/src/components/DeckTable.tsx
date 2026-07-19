/**
 * DeckTable — the active hero's full card table, laid out left-to-right as
 * Scarti/Mazzo/Mano/Carta Selezionata/Giocate/Memoria/Bandite (Scarti sits
 * immediately left of Mazzo since they are the same physical pile viewed
 * from its two ends), replacing Phase 4's single-zone `HandPanel`.
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
 *   Everything else (Memory/PlayArea as a move SOURCE, or as a destination
 *   other than the two above) is a no-op on the real engine —
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
 *     - renders BanishZone (Bandite) as a plain button, NOT a drop zone:
 *       nothing in Eldhôm's command/event set ever populates it, and a
 *       proper banned-cards management UI is a separate, not-yet-built
 *       page — the button (`onOpenBanish`) is just a placeholder hook for
 *       that future page.
 *
 * Interaction: both native HTML5 drag&drop (`draggable`, `onDragStart`,
 * `onDragOver`, `onDrop`) AND plain click/button alternatives are wired for
 * every action (per the redesign's explicit "both" decision) — drag&drop
 * for the tactile hand→zone moves, small buttons for the GM-override
 * actions (discard/draw/take/reshuffle) that don't have an obvious card to
 * grab (e.g. "draw" starts from a hidden, identity-less deck).
 *
 * "Carta Selezionata" (between Mano and Giocate) is not a real deck zone —
 * it is a full "face up" preview of whichever card was last hovered/focused
 * anywhere in the table (hand/played/discard), rendered like a physical
 * game card (icon, name, timeline cost, effect icons, extended description,
 * tags) rather than the one-line header hint it replaces.
 */
import { useState } from 'react'
import { CARD_TYPE_ICONS, effectSummaryLine, hasEffect, isPlayable } from '../engine/cardIcons'
import type { CardWire, HeroWire } from '../engine/contract'
import { readDragPayload, setDragPayload } from '../engine/dragPayload'

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
  onOpenBanish: () => void
}

/** True if the card needs a destination/target and must therefore be dropped on the Map, not on Giocate/Memoria. */
function needsMapDrop(card: CardWire | undefined): boolean {
  if (!card) {
    return false
  }
  return hasEffect(card, 'MOVE', 'MOVE_TOWARD_PG') || hasEffect(card, 'DAMAGE', 'DEAL_DAMAGE')
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
  onOpenBanish,
}: DeckTableProps) {
  const [selectedCardId, setSelectedCardId] = useState<string | null>(null)
  const [dragOverZone, setDragOverZone] = useState<string | null>(null)

  const deckCount = hero?.deck_count ?? 0
  const discardIds = hero?.discard_ids ?? []
  const discardCount = hero?.discard_count ?? discardIds.length
  const playedIds = hero?.played_ids ?? []
  // Wire order is oldest-first/newest-last (index 0 = first discarded). The
  // pile is rendered as a real LIFO stack: reversed so the most recently
  // discarded card (the pile's actual top) is the first DOM child, which
  // `flex-direction: column` then places visually at the TOP of the zone —
  // the oldest discarded card ends up last, at the bottom.
  const discardIdsTopFirst = [...discardIds].reverse()

  const selectedCard = selectedCardId ? cards[selectedCardId] : undefined

  // Giocate/Memoria only accept cards playable RIGHT NOW that need no destination/target
  // (cards needing one — MOVE/DAMAGE effects — must be dropped on the Map instead, see
  // EldhomMap's onCardDropOnLocation/onCardDropOnToken). Click alone never plays a card
  // any more — see the hand card render below, which has no onClick on its play face.
  function handlePlayZoneDrop(event: React.DragEvent): void {
    event.preventDefault()
    setDragOverZone(null)
    const payload = readDragPayload(event)
    if (!payload || payload.from !== 'CardHand' || !enabled) {
      return
    }
    const card = cards[payload.cardId]
    if (!isPlayable(card, sequenceActive) || needsMapDrop(card)) {
      return
    }
    onPlayCard(payload.cardId)
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
      </div>

      <div className="eldhom-deck-table__zones">
        {/* ── Scarti (DiscardPile) — kept immediately left of Mazzo: same physical pile ── */}
        <div
          className={zoneClass('DiscardPile', 'eldhom-deck-table__zone--discard')}
          onDragOver={allowDrop('DiscardPile')}
          onDragLeave={() => setDragOverZone(null)}
          onDrop={handleDiscardZoneDrop}
        >
          <p className="eldhom-deck-table__zone-title">🗑 Scarti ({discardCount})</p>
          <div className="eldhom-deck-table__cards">
            {discardIdsTopFirst.length === 0 && <span className="eldhom-deck-table__zone-empty">vuoti</span>}
            {discardIdsTopFirst.map((cardId, index) => {
              const card = cards[cardId]
              const isTop = index === 0
              return (
                <div
                  key={`${cardId}-${index}`}
                  className={`eldhom-deck-table__card eldhom-deck-table__card--discard${isTop ? ' eldhom-deck-table__card--top' : ''}`}
                  draggable={enabled && isTop}
                  onDragStart={
                    isTop ? (event) => setDragPayload(event, { cardId, from: 'DiscardPile' }) : undefined
                  }
                  onMouseEnter={() => setSelectedCardId(cardId)}
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
              const dropHint = !playable
                ? ''
                : needsMapDrop(card)
                  ? ' — trascina sulla Mappa per giocarla'
                  : ' — trascina su Giocate per giocarla'
              return (
                <div
                  key={`${cardId}-${index}`}
                  className="eldhom-deck-table__card eldhom-deck-table__card--hand"
                  draggable={playable}
                  onDragStart={(event) => setDragPayload(event, { cardId, from: 'CardHand' })}
                  onMouseEnter={() => setSelectedCardId(cardId)}
                  onFocus={() => setSelectedCardId(cardId)}
                  title={(card?.description ?? cardId) + dropHint}
                >
                  {/* Playing a card is drag&drop-only (Map for MOVE/DAMAGE cards, Giocate/
                      Memoria for the rest) — this face is a static preview, not a button. */}
                  <button type="button" className="eldhom-deck-table__card-play" disabled={!playable} tabIndex={-1}>
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

        {/* ── Carta Selezionata — full "face up" preview of the last hovered/focused card,
             rendered like a real game card (icon, title, cost, effects, extended description) ── */}
        <div className="eldhom-deck-table__zone eldhom-deck-table__zone--preview">
          <p className="eldhom-deck-table__zone-title">🔍 Carta Selezionata</p>
          {selectedCard ? (
            <div
              className={`eldhom-deck-table__preview-card eldhom-deck-table__preview-card--${selectedCard.card_type.toLowerCase()}`}
            >
              <div className="eldhom-deck-table__preview-card-header">
                <span className="eldhom-deck-table__preview-card-icon">
                  {CARD_TYPE_ICONS[selectedCard.card_type]}
                </span>
                <span className="eldhom-deck-table__preview-card-name">{selectedCard.name}</span>
                <span className="eldhom-deck-table__preview-card-cost">⏳{selectedCard.timeline_cost}</span>
              </div>
              {selectedCard.origin && (
                <p className="eldhom-deck-table__preview-card-origin">{selectedCard.origin}</p>
              )}
              <div className="eldhom-deck-table__preview-card-effects">
                {selectedCard.effects.map((effect, index) => (
                  <span key={index} className="eldhom-deck-table__preview-card-effect">
                    {effectSummaryLine(effect, selectedCard.timeline_cost)}
                  </span>
                ))}
              </div>
              {selectedCard.description && (
                <p className="eldhom-deck-table__preview-card-desc">{selectedCard.description}</p>
              )}
              {(selectedCard.requires_frontline || selectedCard.reaction_trigger || selectedCard.condition) && (
                <div className="eldhom-deck-table__preview-card-tags">
                  {selectedCard.requires_frontline && (
                    <span className="eldhom-deck-table__preview-card-tag">👤 Prima Linea</span>
                  )}
                  {selectedCard.reaction_trigger && (
                    <span className="eldhom-deck-table__preview-card-tag">
                      ↩ Reazione: {selectedCard.reaction_trigger}
                    </span>
                  )}
                  {selectedCard.condition && (
                    <span className="eldhom-deck-table__preview-card-tag">⚑ {selectedCard.condition}</span>
                  )}
                </div>
              )}
            </div>
          ) : (
            <span className="eldhom-deck-table__zone-empty eldhom-deck-table__preview-card-hint">
              Passa il mouse su una carta per vederla qui come una vera carta da gioco.
            </span>
          )}
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
                  onMouseEnter={() => setSelectedCardId(cardId)}
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

        {/* ── Memoria (Memory) + Bandite (BanishZone) — share one grid column, split
             top/bottom: Memoria stays an alternate play drop target on top; Bandite
             (shortcut button to a future dedicated management page, not yet implemented)
             sits below it — frees the column Bandite used to occupy on its own ── */}
        <div className="eldhom-deck-table__zone-pair">
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
          <div className="eldhom-deck-table__zone eldhom-deck-table__zone--banish">
            <button
              type="button"
              className="eldhom-deck-table__btn"
              onClick={onOpenBanish}
              title="Apre la gestione delle carte bandite (pagina dedicata, in arrivo)"
            >
              🚫 Bandite
            </button>
          </div>
        </div>
      </div>
    </div>
  )
}
