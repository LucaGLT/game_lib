import { useState } from 'react'
import type { EnvelopeRouter } from '../session/EnvelopeRouter'
import { useGmGuiModule } from '../modules/useGmGuiModule'

const EVT_ACTOR_SNAPSHOT = 'gmActor.snapshot'
const EVT_STATUS_ADDED = 'gmActor.actor.status_added'
const EVT_STATUS_REMOVED = 'gmActor.actor.status_removed'

export interface ActorBadgeConfig {
  actorId: string
  label: string
}

export interface StatusBadgeStyle {
  text: string
  variant: string
}

export interface ActorStatusBadgesProps {
  /** Shared router for the active session, or `null` before one exists. */
  router: EnvelopeRouter | null
  /** Actors to render, in display order. */
  actors: readonly ActorBadgeConfig[]
  /** Given the current status set for one actor, returns what its badge shows. */
  resolveBadge: (statuses: readonly string[]) => StatusBadgeStyle
}

interface ActorSnapshotEntry {
  actor_id: string
  statuses?: string[]
}

/**
 * Generic per-actor status badge row, fed directly from `gmActor.*` engine
 * events via the shared `EnvelopeRouter` — web equivalent of the desktop's
 * generic `GmActorModule` dock (independent of any specific game's board).
 * This is a self-subscribing module: it owns its own state instead of
 * relying on a parent's reducer, exactly like `GmActorModule` coexists
 * independently alongside a game-specific board module on the desktop.
 */
export function ActorStatusBadges({ router, actors, resolveBadge }: ActorStatusBadgesProps) {
  const [statusesByActor, setStatusesByActor] = useState<Record<string, string[]>>({})

  useGmGuiModule(
    router,
    { subscribedTypeIds: [EVT_ACTOR_SNAPSHOT, EVT_STATUS_ADDED, EVT_STATUS_REMOVED] },
    (envelope) => {
      const data = envelope.data
      if (envelope.typeId === EVT_ACTOR_SNAPSHOT) {
        const list = (data.actors as ActorSnapshotEntry[] | undefined) ?? []
        const next: Record<string, string[]> = {}
        for (const actor of list) {
          next[actor.actor_id] = actor.statuses ?? []
        }
        setStatusesByActor(next)
      } else if (envelope.typeId === EVT_STATUS_ADDED) {
        const actorId = String(data.actor_id ?? '')
        const status = String(data.status ?? '')
        setStatusesByActor((previous) => {
          const current = previous[actorId] ?? []
          return current.includes(status)
            ? previous
            : { ...previous, [actorId]: [...current, status] }
        })
      } else if (envelope.typeId === EVT_STATUS_REMOVED) {
        const actorId = String(data.actor_id ?? '')
        const status = String(data.status ?? '')
        setStatusesByActor((previous) => ({
          ...previous,
          [actorId]: (previous[actorId] ?? []).filter((existing) => existing !== status),
        }))
      }
    },
  )

  return (
    <div className="gmgui-actor-badges">
      {actors.map(({ actorId, label }) => {
        const badge = resolveBadge(statusesByActor[actorId] ?? [])
        return (
          <span key={actorId} className={`gmgui-actor-badge gmgui-actor-badge--${badge.variant}`}>
            {label}: {badge.text}
          </span>
        )
      })}
    </div>
  )
}
