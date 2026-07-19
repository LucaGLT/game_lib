/**
 * HeroSelectModal — "pick your PG/hero" screen, shown both when creating a
 * new shared-multiplayer session (every roster hero selectable) and when
 * joining one via a code (only the still-free heroes selectable, taken ones
 * shown disabled with who holds them) — see App.tsx's `handleStartMission`/
 * `handleJoinSession`. Mirrors MissionSelectModal's structure/conventions.
 */
import { useEffect, useState } from 'react'
import { Modal } from '@webgui/components/Modal'
import type { PgRosterEntry } from '../engine/contract'

export interface HeroSelectModalProps {
  heroes: PgRosterEntry[]
  /** hero_id -> username currently holding it, or null/absent if free. Omit entirely (create flow) to make every hero selectable. */
  takenBy?: Record<string, string | null>
  title?: string
  onSelect: (heroId: string) => void
  onDismiss?: () => void
}

export function HeroSelectModal({
  heroes,
  takenBy,
  title = 'Scegli il tuo PG',
  onSelect,
  onDismiss,
}: HeroSelectModalProps) {
  const isFree = (heroId: string): boolean => !takenBy || !takenBy[heroId]
  const firstFree = heroes.find((hero) => isFree(hero.hero_id))?.hero_id ?? ''
  const [selectedId, setSelectedId] = useState(firstFree)

  // `heroes`/`takenBy` typically arrive asynchronously (after a mission/preview
  // fetch), after this component's first render — re-sync the default
  // selection once real data lands (see MissionSelectModal for the same gotcha).
  useEffect(() => {
    if (selectedId === '' || !isFree(selectedId)) {
      const nextFree = heroes.find((hero) => isFree(hero.hero_id))?.hero_id ?? ''
      if (nextFree !== selectedId) {
        setSelectedId(nextFree)
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [heroes, takenBy])

  return (
    <Modal title={title} onDismiss={onDismiss}>
      <ul className="eldhom-modal__mission-list">
        {heroes.map((hero) => {
          const holder = takenBy?.[hero.hero_id] ?? null
          const taken = holder !== null && holder !== undefined
          return (
            <li key={hero.hero_id}>
              <button
                type="button"
                disabled={taken}
                className={hero.hero_id === selectedId ? 'eldhom-modal__mission--selected' : ''}
                onClick={() => setSelectedId(hero.hero_id)}
                onDoubleClick={() => !taken && onSelect(hero.hero_id)}
              >
                {hero.display_name}
                {hero.class_name ? ` — ${hero.class_name}` : ''}
                {taken ? ` (già scelto da ${holder})` : ''}
              </button>
            </li>
          )
        })}
        {heroes.length === 0 && <li className="eldhom-modal__empty">Nessun PG disponibile.</li>}
      </ul>
      <div className="eldhom-modal__actions">
        <button type="button" disabled={selectedId === ''} onClick={() => onSelect(selectedId)}>
          OK
        </button>
      </div>
    </Modal>
  )
}
