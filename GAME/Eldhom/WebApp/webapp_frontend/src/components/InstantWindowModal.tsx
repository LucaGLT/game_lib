/**
 * InstantWindowModal — checkbox list of playable INSTANT cards. Port of
 * GAME/Eldhom/GUI/widgets/instant_window_dialog.py's `InstantWindowDialog`.
 *
 * Serves BOTH variants described in GAME/Eldhom/WebApp/PLAN.md, Phase 6 —
 * proactive (opened after an attack is declared) and reactive/"Assestarsi"
 * (opened when an enemy approaches) — with identical content; the engine
 * itself sends both through the same `eldhom.instant.window_opened` event
 * (confirmed by reading GAME/Eldhom/CoreEngine/main.cpp: the reactive case
 * calls the exact same `emit_instant_window()` as the proactive one). The
 * caller distinguishes them via `InstantWindowWire.trigger` to decide
 * whether to send `eldhom.play_instants` or `eldhom.play_reactive_instants`
 * on confirm — this component itself has no opinion on which.
 */
import { useState } from 'react'
import { Modal } from '@webgui/components/Modal'
import type { InstantOptionWire } from '../engine/contract'

export interface InstantWindowModalProps {
  options: InstantOptionWire[]
  /** actor_id -> display name lookup (e.g. hero names, monster token labels). */
  actorNames: Record<string, string>
  onConfirm: (selected: InstantOptionWire[]) => void
}

export function InstantWindowModal({ options, actorNames, onConfirm }: InstantWindowModalProps) {
  const [selected, setSelected] = useState<Set<number>>(new Set())

  function toggle(index: number): void {
    setSelected((previous) => {
      const next = new Set(previous)
      if (next.has(index)) {
        next.delete(index)
      } else {
        next.add(index)
      }
      return next
    })
  }

  return (
    <Modal title="Carte Istantanee">
      <p className="eldhom-modal__intro">
        Sono giocabili le seguenti Carte Istantanee. Seleziona quali giocare (anche nessuna):
      </p>
      <ul className="eldhom-modal__checklist">
        {options.map((option, index) => (
          <li key={`${option.actor_id}-${option.card_id}-${index}`}>
            <label>
              <input
                type="checkbox"
                checked={selected.has(index)}
                onChange={() => toggle(index)}
              />
              {actorNames[option.actor_id] ?? option.actor_id} — {option.card_name}
            </label>
          </li>
        ))}
      </ul>
      <div className="eldhom-modal__actions">
        <button type="button" onClick={() => onConfirm([])}>
          Nessuna
        </button>
        <button
          type="button"
          onClick={() => onConfirm(options.filter((_option, index) => selected.has(index)))}
        >
          Gioca selezionate
        </button>
      </div>
    </Modal>
  )
}
