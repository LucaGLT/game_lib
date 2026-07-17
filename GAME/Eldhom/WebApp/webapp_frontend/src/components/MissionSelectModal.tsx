/**
 * MissionSelectModal — mission picker shown before a session starts. Port
 * of GAME/Eldhom/GUI/app/mission_select_dialog.py's `MissionSelectDialog`,
 * using `GET /missions` (server-side scan) instead of a local filesystem
 * scan — the browser cannot read the server's disk directly.
 *
 * No `onDismiss`: unlike the desktop (where Cancel quits the app), the web
 * page simply has no game to show until a mission is chosen, so there is
 * nothing meaningful to dismiss to.
 */
import { useEffect, useState } from 'react'
import { Modal } from '@webgui/components/Modal'
import type { MissionSummary } from '../engine/contract'

export interface MissionSelectModalProps {
  missions: MissionSummary[]
  onSelect: (missionId: string) => void
}

export function MissionSelectModal({ missions, onSelect }: MissionSelectModalProps) {
  const [selectedId, setSelectedId] = useState(missions[0]?.mission_id ?? '')

  // `missions` typically arrives asynchronously (GET /missions), after this
  // component's first render — the useState initializer above only runs
  // once, so it cannot pick up a default once the list loads. Fall back to
  // the first mission whenever the current selection is no longer valid.
  useEffect(() => {
    if (missions.length > 0 && !missions.some((mission) => mission.mission_id === selectedId)) {
      setSelectedId(missions[0].mission_id)
    }
  }, [missions, selectedId])

  const selected = missions.find((mission) => mission.mission_id === selectedId) ?? null

  return (
    <Modal title="Scegli Missione — Le Pergamene di Eldhôm">
      <p className="eldhom-modal__intro">Missioni disponibili:</p>
      <ul className="eldhom-modal__mission-list">
        {missions.map((mission) => (
          <li key={mission.mission_id}>
            <button
              type="button"
              className={
                mission.mission_id === selectedId ? 'eldhom-modal__mission--selected' : ''
              }
              onClick={() => setSelectedId(mission.mission_id)}
              onDoubleClick={() => onSelect(mission.mission_id)}
            >
              {mission.title} — {mission.mission_id}
            </button>
          </li>
        ))}
        {missions.length === 0 && <li className="eldhom-modal__empty">Nessuna missione trovata.</li>}
      </ul>
      {selected && <p className="eldhom-modal__hint">{selected.description}</p>}
      <div className="eldhom-modal__actions">
        <button type="button" disabled={selectedId === ''} onClick={() => onSelect(selectedId)}>
          OK
        </button>
      </div>
    </Modal>
  )
}
