/**
 * HandPanel — the active hero's hand as a row of clickable card buttons.
 * Port of GAME/Eldhom/GUI/widgets/hand_widget.py's HandWidget, using the
 * type/effect icon logic from GAME/Eldhom/GUI/app/eldhom_main_window.py
 * (ported to engine/cardIcons.ts).
 *
 * Playability hint (SINGLE/SEQ_START only with no sequence active,
 * SEQ_CONTINUE/SEQ_END only while one is active, INSTANT never from here —
 * played only via reaction windows, see Phase 6) is a convenience UI hint,
 * not strict validation: the engine is always the final authority (an
 * illegal play just yields `eldhom.action.result: {ok:false}`) — the
 * desktop `HandWidget` itself relies on the same engine-side validation
 * (it only has one blanket enabled/disabled flag, no per-card gating).
 */
import { CARD_TYPE_ICONS, effectSummaryLine } from '../engine/cardIcons'
import type { CardWire } from '../engine/contract'

export interface HandPanelProps {
  heroName: string
  cardIds: string[]
  cards: Record<string, CardWire>
  sequenceActive: boolean
  enabled: boolean
  onPlayCard: (cardId: string) => void
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

export function HandPanel({
  heroName,
  cardIds,
  cards,
  sequenceActive,
  enabled,
  onPlayCard,
}: HandPanelProps) {
  return (
    <div className="eldhom-hand">
      <p className="eldhom-hand__title">
        Mano — {heroName}
        {cardIds.length === 0 ? ' (mano vuota)' : ` (${cardIds.length} carte)`}
      </p>
      <div className="eldhom-hand__cards">
        {cardIds.map((cardId, index) => {
          const card = cards[cardId]
          const playable = enabled && isPlayable(card, sequenceActive)
          const typeIcon = card ? CARD_TYPE_ICONS[card.card_type] : '📄'
          const summary = card
            ? card.effects.map((effect) => effectSummaryLine(effect, card.timeline_cost)).join('  ')
            : ''
          return (
            <button
              key={`${cardId}-${index}`}
              type="button"
              className="eldhom-hand__card"
              disabled={!playable}
              title={card?.description ?? cardId}
              onClick={() => onPlayCard(cardId)}
            >
              <span className="eldhom-hand__card-name">
                {typeIcon} {card?.name ?? cardId}
              </span>
              <span className="eldhom-hand__card-summary">{summary}</span>
            </button>
          )
        })}
      </div>
    </div>
  )
}
