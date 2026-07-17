import { THEMES, type ThemeId } from '../theme/themes'

export interface ThemeSelectProps {
  themeId: ThemeId
  onThemeChange: (themeId: ThemeId) => void
  label?: string
}

/** Generic theme picker — reusable by any game's toolbar (the 5 shared gmGui themes). */
export function ThemeSelect({ themeId, onThemeChange, label = 'Tema' }: ThemeSelectProps) {
  return (
    <label className="gmgui-field">
      {label}
      <select value={themeId} onChange={(event) => onThemeChange(event.target.value as ThemeId)}>
        {THEMES.map((theme) => (
          <option key={theme.id} value={theme.id}>
            {theme.displayName}
          </option>
        ))}
      </select>
    </label>
  )
}
