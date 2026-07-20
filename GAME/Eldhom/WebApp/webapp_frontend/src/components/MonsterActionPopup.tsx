/**
 * MonsterActionPopup — non-interactive notification shown while the engine
 * paces a monster group's turn one action at a time (see
 * GAME/Eldhom/CoreEngine/main.cpp's `advance_auto()` /
 * `EVT_MONSTER_ACTION_POPUP`). Purely informational: there is no choice to
 * make, only a dismissal.
 *
 * Closes when: the player clicks anywhere on it, another connected client
 * dismisses it, or a server-side 3-second timeout elapses with nobody
 * acknowledging it. All three routes go through the SAME
 * `eldhom.ack_monster_popup` command / `EVT_MONSTER_POPUP_CLOSED` (or the
 * next `EVT_MONSTER_ACTION_POPUP`) event round-trip — the server is the only
 * authority on when it actually closes, so every client stays in sync.
 */
import { Modal } from '@webgui/components/Modal'
import type { MonsterActionPopupWire } from '../engine/contract'

export interface MonsterActionPopupProps {
  popup: MonsterActionPopupWire
  onAck: () => void
}

export function MonsterActionPopup({ popup, onAck }: MonsterActionPopupProps) {
  return (
    <Modal title="Azione Mostro" onDismiss={onAck}>
      <div className="eldhom-monster-popup" onClick={onAck}>
        <p className="eldhom-modal__intro">{popup.description}</p>
        <p className="eldhom-modal__hint">Clicca per continuare…</p>
      </div>
    </Modal>
  )
}
