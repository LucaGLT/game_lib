/**
 * MainMenuModal — landing screen shown on startup, before any mission is
 * chosen and before a session exists. Requested explicitly so the app no
 * longer jumps straight into `MissionSelectModal` on load: the player picks
 * one of the 4 top-level sections first (theme switching, via `ThemeSelect`
 * in `App.tsx`'s header, already works from this screen since the header
 * renders regardless of session state).
 *
 * Only "Gioca una missione" is wired up for now — it hands off to the
 * existing `MissionSelectModal` flow. The other 3 entries are real, visible
 * placeholders (disabled buttons with a hint) until their own screens
 * exist; there is deliberately no dead `onClick` wired to them.
 */
import { Modal } from '@webgui/components/Modal'

export interface MainMenuModalProps {
  onPlayMission: () => void
}

interface MainMenuEntry {
  readonly key: string
  readonly label: string
  readonly hint: string
}

/** The 3 not-yet-implemented entries — kept in one place so adding a real screen later is a one-line removal here. */
const COMING_SOON_ENTRIES: readonly MainMenuEntry[] = [
  { key: 'village', label: '🏘 Esplora un villaggio', hint: 'Non ancora disponibile' },
  { key: 'roster', label: '🧑\u200d🤝\u200d🧑 Gestisci PG', hint: 'Non ancora disponibile' },
  { key: 'settings', label: '⚙ Settings', hint: 'Non ancora disponibile' },
]

export function MainMenuModal({ onPlayMission }: MainMenuModalProps) {
  return (
    <Modal title="Le Pergamene di Eldhôm">
      <ul className="eldhom-modal__main-menu">
        <li>
          <button type="button" onClick={onPlayMission}>
            ⚔ Gioca una missione
          </button>
        </li>
        {COMING_SOON_ENTRIES.map((entry) => (
          <li key={entry.key}>
            <button type="button" disabled title={entry.hint}>
              {entry.label}
            </button>
            <span className="eldhom-modal__main-menu-hint">{entry.hint}</span>
          </li>
        ))}
      </ul>
    </Modal>
  )
}
