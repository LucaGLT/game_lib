/**
 * MonsterGroupPanel — compact synthetic card for one monster group (HP
 * aggregate across all instances, alive/total count, timeline), shown
 * alongside `HeroPanel` in `App.tsx`'s hero row (Phase 20, explicit user
 * request: "voglio vedere anche le Card Sintetiche dei Gruppi di Mostri in
 * gioco oltre alla Card Sintetiche dei PG").
 *
 * One card per GROUP, not per instance — mirrors how `TimelineTrack` shows
 * one chip per group rather than per monster instance. Reuses `HeroPanel`'s
 * `.eldhom-hero-panel` CSS classes (same "card" chrome) plus a `--monster`
 * modifier for the red/danger accent, same convention as
 * `TimelineTrack`'s `--enemy` chip.
 *
 * Clicking the card calls `onClick` — `App.tsx` opens `ActorDetailModal`
 * with the full per-instance breakdown, the "extended card" equivalent of
 * the desktop's `GmActorModule` detail panel shown when an actor is
 * selected there.
 */
import type { MonsterGroupWire } from '../engine/contract'

export interface MonsterGroupPanelProps {
  group: MonsterGroupWire
  isActive: boolean
  onClick: () => void
}

export function MonsterGroupPanel({ group, isActive, onClick }: MonsterGroupPanelProps) {
  const aliveCount = group.instances.filter((instance) => instance.alive).length
  const totalCount = group.instances.length
  const totalHp = group.instances.reduce((sum, instance) => sum + Math.max(0, instance.hp), 0)
  const totalMaxHp = group.instances.reduce((sum, instance) => sum + instance.max_hp, 0)
  const hpRatio = totalMaxHp > 0 ? totalHp / totalMaxHp : 0

  return (
    <div
      className={`eldhom-hero-panel eldhom-hero-panel--monster eldhom-hero-panel--clickable${isActive ? ' eldhom-hero-panel--active' : ''}`}
      onClick={onClick}
    >
      <p className="eldhom-hero-panel__name">👹 {group.name}</p>
      <div className="eldhom-hero-panel__hp-track">
        <div className="eldhom-hero-panel__hp-fill" style={{ width: `${hpRatio * 100}%` }} />
        <span className="eldhom-hero-panel__hp-text">
          ❤ {totalHp}/{totalMaxHp}
        </span>
      </div>
      <p className="eldhom-hero-panel__status">
        {aliveCount}/{totalCount} vivi
      </p>
      <p className="eldhom-hero-panel__timeline">⌛ {group.timeline}</p>
      <p className="eldhom-hero-panel__location">📍 {group.location}</p>
    </div>
  )
}
