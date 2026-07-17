/**
 * FormationModal — mandatory Prima Linea/Retroguardia assignment dialog.
 * Port of GAME/Eldhom/GUI/widgets/formation_dialog.py's `FormationDialog`.
 * Used for Scompaginamento (formation violation) and Schieramento
 * (deliberate formation choice), including DISRUPT_ENEMY_FORMATION card
 * effects and DODGE reactions that invalidate the enemy formation.
 *
 * No `onDismiss` is passed to `Modal` here: resolving the formation is
 * mandatory (mirrors the desktop dialog, which has no Cancel button).
 */
import { useState } from 'react'
import { Modal } from '@webgui/components/Modal'
import type { FormationActorWire } from '../engine/contract'

export interface FormationModalProps {
  locationId: string
  factionId: string
  source: string
  actors: FormationActorWire[]
  onConfirm: (backlineActorIds: string[]) => void
}

const SOURCE_LABELS: Record<string, string> = {
  scompaginamento: '⚠ Scompaginamento: la Prima Linea è vuota.',
  overflow: '⚠ Formazione non valida: troppi in Retroguardia.',
  disrupt: '⚔ Scompaginamento (da carta): riorganizza la formazione nemica.',
}

export function FormationModal({
  locationId,
  factionId,
  source,
  actors,
  onConfirm,
}: FormationModalProps) {
  const [backline, setBackline] = useState<Set<string>>(
    () => new Set(actors.filter((actor) => actor.in_backline).map((actor) => actor.actor_id)),
  )

  const backlineCount = backline.size
  const frontlineCount = actors.length - backlineCount
  const valid = backlineCount <= frontlineCount

  function toggle(actorId: string): void {
    setBackline((previous) => {
      const next = new Set(previous)
      if (next.has(actorId)) {
        next.delete(actorId)
      } else {
        next.add(actorId)
      }
      return next
    })
  }

  return (
    <Modal title={`Formazione — ${factionId} @ ${locationId}`}>
      <p className="eldhom-modal__intro">{SOURCE_LABELS[source] ?? 'Decidi la Formazione:'}</p>
      <p className="eldhom-modal__hint">
        ☑ spunta = Retroguardia&nbsp;&nbsp;☐ deseleziona = Prima Linea (Retroguardia ≤ Prima
        Linea)
      </p>
      <ul className="eldhom-modal__checklist">
        {actors.map((actor) => (
          <li key={actor.actor_id}>
            <label>
              <input
                type="checkbox"
                checked={backline.has(actor.actor_id)}
                onChange={() => toggle(actor.actor_id)}
              />
              {actor.name} → Retroguardia
            </label>
          </li>
        ))}
      </ul>
      {!valid && (
        <p className="eldhom-modal__error">
          ⚠ Retroguardia ({backlineCount}) &gt; Prima Linea ({frontlineCount}): sposta qualcuno
          in Prima Linea.
        </p>
      )}
      <div className="eldhom-modal__actions">
        <button type="button" disabled={!valid} onClick={() => onConfirm([...backline])}>
          OK
        </button>
      </div>
    </Modal>
  )
}
