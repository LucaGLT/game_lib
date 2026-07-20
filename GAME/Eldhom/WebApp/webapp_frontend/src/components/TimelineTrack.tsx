/**
 * TimelineTrack — horizontal strip of chips showing every actor sorted by
 * activation timeline position, with the currently-acting actor
 * highlighted. Started as a port of
 * `GAME/Eldhom/GUI/widgets/timeline_widget.py`'s `TimelineWidget`/
 * `_TimelineChip` (desktop QSS-styled chips) to React/CSS.
 *
 * Migliorie Grafiche 01 (explicit user request): each chip now ALSO carries
 * everything the separate hero/monster-group card row used to show (PG vs
 * Mostri icon, HP bar, alive-count for monster groups, location+Prima
 * Linea/Retroguardia for heroes only) and is clickable to open that actor's
 * "extended card" (`ActorDetailModal`, via `onActorClick`) — the old
 * `HeroPanel`/`MonsterGroupPanel` row below the map was removed entirely
 * since this tile now carries the same information. The sorted horizontal
 * strip mechanism itself is UNCHANGED, per the user's explicit request to
 * leave the Timeline "exactly as it is managed now".
 *
 * Eldhôm-specific (not in `webLib/WebGUI_Lib`): the concept of a sorted
 * "activation order" strip is not yet needed by any other game_lib WebApp.
 * Revisit generalizing it only once a second consumer actually needs it
 * (see GAME/Eldhom/WebApp/PLAN.md, Phase 3 Notes).
 */
import { POSITION_ICONS } from '../engine/cardIcons'
import type { TimelineActor } from '../engine/gameState'

export interface TimelineTrackProps {
  actors: TimelineActor[]
  activeActorId: string
  /** Called with an actor's id and whether it is a hero (vs a monster group) when its tile is clicked. */
  onActorClick: (actorId: string, isHero: boolean) => void
}

export function TimelineTrack({ actors, activeActorId, onActorClick }: TimelineTrackProps) {
  const sorted = [...actors].sort((a, b) => a.timeline - b.timeline)

  return (
    <div className="eldhom-timeline" role="list" aria-label="Linea Temporale">
      {sorted.map((actor) => {
        const hpRatio = actor.maxHp > 0 ? Math.max(0, actor.hp) / actor.maxHp : 0
        return (
          <div
            key={actor.actorId}
            role="listitem"
            className={[
              'eldhom-timeline__chip',
              actor.isHero ? 'eldhom-timeline__chip--hero' : 'eldhom-timeline__chip--enemy',
              actor.actorId === activeActorId ? 'eldhom-timeline__chip--active' : '',
            ]
              .filter(Boolean)
              .join(' ')}
            onClick={() => onActorClick(actor.actorId, actor.isHero)}
          >
            <div className="eldhom-timeline__chip-header">
              <span className="eldhom-timeline__chip-icon" aria-hidden="true">
                {actor.isHero ? '👤' : '👹'}
              </span>
              <span className="eldhom-timeline__chip-name">{actor.name}</span>
            </div>
            <div className="eldhom-timeline__chip-hp-track">
              <div className="eldhom-timeline__chip-hp-fill" style={{ width: `${hpRatio * 100}%` }} />
              <span className="eldhom-timeline__chip-hp-text">
                ❤ {Math.max(0, actor.hp)}/{actor.maxHp}
              </span>
            </div>
            {!actor.isHero && (
              <span className="eldhom-timeline__chip-alive">
                {actor.aliveCount}/{actor.totalCount} vivi
              </span>
            )}
            <span className="eldhom-timeline__chip-time">⏳{actor.timeline}</span>
            {actor.isHero && (
              <span className="eldhom-timeline__chip-location">
                📍 {actor.location} {POSITION_ICONS[actor.position]}
              </span>
            )}
          </div>
        )
      })}
    </div>
  )
}

