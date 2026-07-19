/**
 * MissionDetailsModal — pop-up shown when the player clicks the mission
 * header ("SessionCode : XXXXXX -- Missione : <titolo>"). Explicit user
 * request: the header itself must never show the mission's elapsed time
 * (⏳) any more, only the session code and title — the time, plus the full
 * description (from the mission's JSON, via `GET /missions`), move here
 * instead, one click away.
 */
import { Modal } from '@webgui/components/Modal'

export interface MissionDetailsModalProps {
  title: string
  joinCode: string
  description: string
  onDismiss: () => void
}

export function MissionDetailsModal({ title, joinCode, description, onDismiss }: MissionDetailsModalProps) {
  return (
    <Modal title="Dettagli Missione" onDismiss={onDismiss}>
      <div className="eldhom-mission-detail">
        <div className="eldhom-mission-detail__section">
          <p className="eldhom-mission-detail__section-title">Titolo</p>
          <p>{title}</p>
        </div>
        <div className="eldhom-mission-detail__section">
          <p className="eldhom-mission-detail__section-title">Codice Sessione</p>
          <p>
            <span className="eldhom-mission-detail__code">{joinCode}</span>
          </p>
        </div>
        <div className="eldhom-mission-detail__section">
          <p className="eldhom-mission-detail__section-title">Descrizione</p>
          <p>{description || 'Nessuna descrizione disponibile.'}</p>
        </div>
      </div>
    </Modal>
  )
}
