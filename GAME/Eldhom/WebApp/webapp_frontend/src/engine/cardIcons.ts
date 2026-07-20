/**
 * cardIcons — type/effect icon+summary logic for hand cards. Direct port of
 * GAME/Eldhom/GUI/app/eldhom_main_window.py's `_CARD_TYPE_ICONS`/
 * `_EFFECT_ICONS`/`_ATTACK_TYPE_ICONS`/`_POSITION_ICONS`/
 * `_effect_summary_line()` (itself following UI-Standard-Carte-Missione.md
 * §5). Only the compact one-line-per-effect summary is ported (used by
 * `HandPanel`) — the full rich-text `_card_description()` detail block is
 * not needed by Phase 4's scope.
 */
import type { CardEffectWire, CardType, CardWire } from './contract'

/** True if *card* can currently be played from hand, given whether the hero has an open sequence (SINGLE/SEQ_START need none open; SEQ_CONTINUE/SEQ_END need one open; INSTANT is never played from hand). */
export function isPlayable(card: CardWire | undefined, sequenceActive: boolean): boolean {
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

/** §5.2 — Tipo di uso. */
export const CARD_TYPE_ICONS: Record<CardType, string> = {
  SINGLE: '📄',
  INSTANT: '⚡',
  SEQ_START: '🟢',
  SEQ_CONTINUE: '🟡',
  SEQ_END: '🔴',
}

/** §5.1 — Azioni principali (effect_type -> icona). */
const EFFECT_ICONS: Record<string, string> = {
  MOVE: '▶️',
  MOVE_TOWARD_PG: '▶️',
  DAMAGE: '⏸️',
  DEAL_DAMAGE: '⏸️',
  INTERACT: '⏺️',
  REDUCE_DAMAGE: '🛡️',
  HEAL: '❤️',
  FORMATION_PUSH: '🤝',
  DISRUPT_ENEMY_FORMATION: '🤝',
  DRAW_CARD: '🧠',
  WAIT: '⏺️',
  DISCARD_THEN_DRAW: '🔄',
}

/** §5.3 — Tipo di attacco. */
const ATTACK_TYPE_ICONS: Record<string, string> = {
  MELEE: '⚔️',
  RANGED: '🏹',
}

/** §5.4 — Posizione in formazione. Exported for reuse by `TimelineTrack.tsx` (merged Actor tile). */
export const POSITION_ICONS: Record<string, string> = {
  FRONTLINE: '👤',
  BACKLINE: '👥',
}

/**
 * Returns a compact icon+text line for one card effect (mirrors
 * `_effect_summary_line`). Examples: "⏸️⚔️ 1❌ : 2⏳", "▶️2◻️ : 2⏳", "+1❤️ : 3⏳".
 */
export function effectSummaryLine(effect: CardEffectWire, cost: number): string {
  const type = effect.effect_type
  const amount = effect.amount
  const icon = EFFECT_ICONS[type] ?? '•'
  const attackType = effect.attack_type ?? 'MELEE'
  const attackIcon = ATTACK_TYPE_ICONS[attackType] ?? '⚔️'
  const costSuffix = cost ? ` : ${cost}⏳` : ''

  if (type === 'MOVE' || type === 'MOVE_TOWARD_PG') {
    return amount ? `${icon}${amount}◻️${costSuffix}` : `${icon}${costSuffix}`
  }
  if (type === 'DAMAGE' || type === 'DEAL_DAMAGE') {
    if (!amount) {
      return `${icon} Attacca${costSuffix}`
    }
    if (attackType === 'RANGED' && effect.range) {
      return `${icon}${attackIcon}${effect.range}◻️ ${amount}❌${costSuffix}`
    }
    return `${icon}${attackIcon} ${amount}❌${costSuffix}`
  }
  if (type === 'HEAL') {
    return amount ? `+${amount}❤️${costSuffix}` : `❤️${costSuffix}`
  }
  if (type === 'REDUCE_DAMAGE') {
    const base = amount ? `${icon} -${amount}❌` : `${icon} Riduzione danno`
    return base + costSuffix
  }
  if (type === 'FORMATION_PUSH') {
    const positionIcon = POSITION_ICONS[(effect.value ?? '').toUpperCase()] ?? icon
    return `${icon}${positionIcon}${costSuffix}`
  }
  if (type === 'DRAW_CARD') {
    return amount ? `${icon}${amount}` : icon
  }
  if (type === 'DISCARD_THEN_DRAW') {
    return amount ? `🔄${amount}` : '🔄'
  }
  if (type === 'INTERACT' || type === 'DISRUPT_ENEMY_FORMATION') {
    return `${icon}${costSuffix}`
  }
  return `${icon} ${type.replaceAll('_', ' ')}${costSuffix}`
}

/** True if any of the card's effects has one of the given effect_type values. */
export function hasEffect(card: CardWire, ...effectTypes: string[]): boolean {
  return card.effects.some((effect) => effectTypes.includes(effect.effect_type))
}
