/**
 * TimelineTrack — horizontal strip of chips showing every actor sorted by
 * activation timeline position, with the currently-acting actor
 * highlighted. Port of `GAME/Eldhom/GUI/widgets/timeline_widget.py`'s
 * `TimelineWidget`/`_TimelineChip` (desktop QSS-styled chips) to
 * React/CSS — same `hero`/`enemy` + active/inactive visual states, driven
 * by the shared `--gm-*` theme variables (the desktop reference is itself
 * theme-reactive via QSS, unlike the map's edge/token colours — see
 * EldhomMap.tsx's docstring).
 *
 * Eldhôm-specific (not in `webLib/WebGUI_Lib`): the concept of a sorted
 * "activation order" strip is not yet needed by any other game_lib WebApp.
 * Revisit generalizing it only once a second consumer actually needs it
 * (see GAME/Eldhom/WebApp/PLAN.md, Phase 3 Notes).
 */
import type { TimelineActor } from '../engine/gameState'

export interface TimelineTrackProps {
  actors: TimelineActor[]
  activeActorId: string
}

export function TimelineTrack({ actors, activeActorId }: TimelineTrackProps) {
  const sorted = [...actors].sort((a, b) => a.timeline - b.timeline)

  return (
    <div className="eldhom-timeline" role="list" aria-label="Linea Temporale">
      {sorted.map((actor) => (
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
        >
          <span className="eldhom-timeline__chip-name">{actor.name}</span>
          <span className="eldhom-timeline__chip-time">⏳{actor.timeline}</span>
        </div>
      ))}
    </div>
  )
}
