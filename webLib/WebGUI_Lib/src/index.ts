/**
 * WebGUI_Lib — public API barrel. Generic, game-agnostic React/TypeScript
 * building blocks shared by every game_lib WebApp: web equivalent of
 * pyLib/gmGui for the PySide6 desktop GUIs.
 *
 * Consumers currently import from specific submodule paths (e.g.
 * `@webgui/theme/themes`, `@webgui/components/ErrorBar`) rather than this
 * barrel, matching the existing per-file import style in each WebApp. This
 * file documents the intended public surface for future consumers/packaging.
 */

export * from './theme/themes'
export * from './session/types'
export * from './session/restClient'
export * from './session/wsClient'
export * from './session/authClient'
export * from './session/AuthProvider'
export * from './session/EnvelopeRouter'
export * from './modules/GmGuiModule'
export * from './modules/useGmGuiModule'
export * from './components/ErrorBar'
export * from './components/EventLog'
export * from './components/ActorStatusBadges'
export * from './components/ThemeSelect'
export * from './components/Modal'
export * from './components/LoginForm'
export * from './components/JoinSessionForm'
