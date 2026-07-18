/**
 * HeroPanel — compact per-hero stat card (HP bar, status, timeline,
 * location/position). Port of
 * GAME/Eldhom/GUI/widgets/hero_panel_widget.py's `HeroPanelWidget`.
 *
 * `ActorStatusBadges` (`@webgui/*`) was evaluated for reuse (per
 * GAME/Eldhom/WebApp/PLAN.md, Phase 5) but declined: it subscribes to
 * generic `gmActor.*` status-effect events, which nothing in Eldhôm's own
 * `eldhom.*` wire contract produces (that translation only exists inside
 * the desktop's Python-side `EldhomActorAdapter`, not in eng_serve's
 * pass-through). The "whose turn is it" information it would otherwise
 * show is already covered by `ActionPanel`'s "TURNO: X" label, this
 * component's own active-border highlight, and `EldhomMap`'s active-token
 * outline — adding it would be redundant, not missing functionality.
 *
 * Clicking the card calls `onClick` (Phase 20) — `App.tsx` opens
 * `ActorDetailModal` with this hero's full wire data, the "extended card"
 * equivalent of the desktop's `GmActorModule` detail panel shown when an
 * actor is selected there.
 */
import type { HeroWire } from '../engine/contract'

export interface HeroPanelProps {
  hero: HeroWire
  isActive: boolean
  onClick: () => void
}

const LIFE_STATE_LABELS: Record<number, string> = {
  0: 'Attivo',
  1: 'KO',
  2: 'Morto',
}

export function HeroPanel({ hero, isActive, onClick }: HeroPanelProps) {
  const hpRatio = hero.max_hp > 0 ? Math.max(0, hero.hp) / hero.max_hp : 0
  const positionLabel = hero.position === 'FRONTLINE' ? 'Primo piano' : 'Retro'

  return (
    <div
      className={`eldhom-hero-panel eldhom-hero-panel--clickable${isActive ? ' eldhom-hero-panel--active' : ''}`}
      onClick={onClick}
    >
      <p className="eldhom-hero-panel__name">{hero.name}</p>
      <div className="eldhom-hero-panel__hp-track">
        <div className="eldhom-hero-panel__hp-fill" style={{ width: `${hpRatio * 100}%` }} />
        <span className="eldhom-hero-panel__hp-text">
          ❤ {Math.max(0, hero.hp)}/{hero.max_hp}
        </span>
      </div>
      <p className="eldhom-hero-panel__status">
        {LIFE_STATE_LABELS[hero.life_state] ?? 'Attivo'} • mazzo:{hero.deck_count}/{hero.hand_limit}
      </p>
      <p className="eldhom-hero-panel__timeline">⌛ {hero.timeline}</p>
      <p className="eldhom-hero-panel__location">
        📍 {hero.location} ({positionLabel})
      </p>
    </div>
  )
}

