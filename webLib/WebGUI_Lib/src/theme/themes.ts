/**
 * themes — the 5 gmGui themes, ported from the PySide6 reference implementation
 * (`pyLib/gmGui/theme_manager.py`) so any game_lib WebApp looks consistent with
 * the desktop GUI. Color/radius values are the *runtime* palette used by
 * `ThemeManager`, which differs slightly from the abstract token file
 * (`.github/specs/gui-theme.yml`) — this module intentionally matches what the
 * desktop app actually renders. Game-agnostic: contains no game-specific text,
 * icons, or hardcoded colors beyond the shared theme palette itself.
 *
 * `displayFont`/`bodyFont` are a WebApp-only addition (the desktop app uses a
 * single fixed QSS font-family for every theme, see `theme_manager.py` line
 * ~210) — each theme pairs a thematic display face (titles/names) with a
 * readable body face (running text), loaded via Google Fonts in
 * `webLib/WebGUI_Lib/src/styles.css`. Every stack ends in a generic
 * (serif/monospace) fallback so the UI stays usable if the web font fails to
 * load (e.g. no network).
 */

export type ThemeId = 'scroll' | 'stone' | 'dark_moon' | 'blood' | 'techno'

export interface ThemeTokens {
  id: ThemeId
  displayName: string
  background: string
  panel: string
  border: string
  accent: string
  text: string
  cornerRadiusPx: number
  /** Thematic font stack for titles/headings/names (short, prominent text). */
  displayFont: string
  /** Thematic font stack for running/body text (labels, descriptions, log). */
  bodyFont: string
}

export const DEFAULT_THEME_ID: ThemeId = 'scroll'

/** Ordered the same way as `ThemeManager.available_themes()`. */
export const THEMES: readonly ThemeTokens[] = [
  {
    id: 'scroll',
    displayName: 'Scroll',
    background: '#F3E9D2',
    panel: '#E9DBBB',
    border: '#8A6A3F',
    accent: '#B88A3D',
    text: '#2F2115',
    cornerRadiusPx: 4,
    // Old-manuscript / parchment writing feel.
    displayFont: "'IM Fell English', Georgia, 'Times New Roman', serif",
    bodyFont: "'EB Garamond', Georgia, 'Times New Roman', serif",
  },
  {
    id: 'stone',
    displayName: 'Stone',
    background: '#BDB8AF',
    panel: '#D2CEC6',
    border: '#59544D',
    accent: '#7A7A6B',
    text: '#1E1E1E',
    cornerRadiusPx: 2,
    // Roman-inscription / carved-rune feel.
    displayFont: "'Cinzel', Georgia, serif",
    bodyFont: "'Crimson Text', Georgia, serif",
  },
  {
    id: 'dark_moon',
    displayName: 'Dark Moon',
    background: '#1A1A1E',
    panel: '#26262D',
    border: '#54546A',
    accent: '#A89CC8',
    text: '#E5E5E5',
    cornerRadiusPx: 6,
    // Gothic fantasy-noir feel (wolves/bats/moonlit ruins).
    displayFont: "'Pirata One', Georgia, serif",
    bodyFont: "'Spectral', Georgia, serif",
  },
  {
    id: 'blood',
    displayName: 'Blood',
    background: '#140A0A',
    panel: '#241111',
    border: '#6B1515',
    accent: '#B52A2A',
    text: '#F2E6E6',
    cornerRadiusPx: 8,
    // Vampiric / gothic-horror-novel feel (Dracula print typography).
    displayFont: "'Metamorphous', Georgia, serif",
    bodyFont: "'Cormorant', Georgia, serif",
  },
  {
    id: 'techno',
    displayName: 'Techno',
    background: '#08131E',
    panel: '#10202D',
    border: '#00C8FF',
    accent: '#00E5FF',
    text: '#D8F8FF',
    cornerRadiusPx: 12,
    // Sci-fi HUD / monitor / virtual-reality terminal feel.
    displayFont: "'Orbitron', 'Consolas', monospace",
    bodyFont: "'Share Tech Mono', 'Consolas', monospace",
  },
]

const THEMES_BY_ID: Record<ThemeId, ThemeTokens> = Object.fromEntries(
  THEMES.map((theme) => [theme.id, theme]),
) as Record<ThemeId, ThemeTokens>

export function getTheme(id: ThemeId): ThemeTokens {
  return THEMES_BY_ID[id] ?? THEMES_BY_ID[DEFAULT_THEME_ID]
}

/** Lightens a `#rrggbb` color by mixing it towards white (mirrors QColor.lighter()). */
function lighten(hex: string, amount: number): string {
  return mix(hex, '#ffffff', amount)
}

/** Darkens a `#rrggbb` color by mixing it towards black (mirrors QColor.darker()). */
function darken(hex: string, amount: number): string {
  return mix(hex, '#000000', amount)
}

function mix(hex: string, towards: string, amount: number): string {
  const a = parseHex(hex)
  const b = parseHex(towards)
  const r = Math.round(a.r + (b.r - a.r) * amount)
  const g = Math.round(a.g + (b.g - a.g) * amount)
  const bl = Math.round(a.b + (b.b - a.b) * amount)
  return `#${[r, g, bl].map((channel) => channel.toString(16).padStart(2, '0')).join('')}`
}

function parseHex(hex: string): { r: number; g: number; b: number } {
  const clean = hex.replace('#', '')
  return {
    r: parseInt(clean.substring(0, 2), 16),
    g: parseInt(clean.substring(2, 4), 16),
    b: parseInt(clean.substring(4, 6), 16),
  }
}

/**
 * Builds the CSS custom properties for one theme, applied on the app root.
 * Tone derivations mirror `ThemeManager._build_stylesheet` (accent.lighter(125)
 * / accent / accent.darker(135) for success/warning/danger).
 *
 * Also sets the actual `fontFamily` CSS property (not just a `--gm-*`
 * custom property) to the theme's body face — since `font-family` is
 * inherited by default, applying this on the SAME root element consuming
 * apps already spread this object onto is enough to theme every descendant
 * "for free", with zero extra CSS in each game. Headings opt into the
 * DISPLAY face instead via a plain `h1..h6`/`.gmgui-display-font` rule in
 * `styles.css`.
 */
export function themeToCssVars(theme: ThemeTokens): Record<string, string> {
  return {
    '--gm-background': theme.background,
    '--gm-panel': theme.panel,
    '--gm-border': theme.border,
    '--gm-accent': theme.accent,
    '--gm-text': theme.text,
    '--gm-radius': `${theme.cornerRadiusPx}px`,
    '--gm-success': lighten(theme.accent, 0.25),
    '--gm-warning': theme.accent,
    '--gm-danger': darken(theme.accent, 0.35),
    '--gm-font-display': theme.displayFont,
    '--gm-font-body': theme.bodyFont,
    fontFamily: theme.bodyFont,
  }
}
