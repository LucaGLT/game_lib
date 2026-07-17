import { ThemeSelect } from '@webgui/components/ThemeSelect'
import type { ThemeId } from '@webgui/theme/themes'

interface GameToolbarProps {
  starterMode: string
  onStarterModeChange: (mode: string) => void
  onNewGame: () => void
  themeId: ThemeId
  onThemeChange: (themeId: ThemeId) => void
}

const STARTER_MODES: Array<{ value: string; label: string }> = [
  { value: 'fixed_x', label: 'Inizio: X (fisso)' },
  { value: 'dice_1d2', label: 'Inizio: Dado 1d2' },
]

/** Top toolbar — mirrors the desktop toolbar (Nuova partita + Modalità) plus the shared theme selector. */
function GameToolbar({
  starterMode,
  onStarterModeChange,
  onNewGame,
  themeId,
  onThemeChange,
}: GameToolbarProps) {
  return (
    <div className="game-toolbar">
      <button type="button" className="game-toolbar__primary" onClick={onNewGame}>
        Nuova Partita
      </button>

      <label className="gmgui-field">
        Modalità
        <select value={starterMode} onChange={(event) => onStarterModeChange(event.target.value)}>
          {STARTER_MODES.map((mode) => (
            <option key={mode.value} value={mode.value}>
              {mode.label}
            </option>
          ))}
        </select>
      </label>

      <ThemeSelect themeId={themeId} onThemeChange={onThemeChange} />
    </div>
  )
}

export default GameToolbar
