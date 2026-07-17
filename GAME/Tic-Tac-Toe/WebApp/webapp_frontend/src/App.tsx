import { useEffect, useRef, useState, type CSSProperties } from 'react'
import { createSession, sendCommand } from './api/restClient'
import { connectSessionEvents } from './api/wsClient'
import { CMD_MOVE, CMD_NEW_GAME } from './engine/contract'
import { applyEnvelope, initialGameState, requestNewGame, type GameState } from './engine/gameState'
import { DEFAULT_THEME_ID, THEMES, getTheme, themeToCssVars, type ThemeId } from './theme/themes'
import GameToolbar from './components/GameToolbar'
import TurnHeader from './components/TurnHeader'
import TrisBoard from './components/TrisBoard'
import MatchLog from './components/MatchLog'
import PlayerBadges from './components/PlayerBadges'
import ErrorBar from './components/ErrorBar'
import './App.css'

const THEME_STORAGE_KEY = 'tris-webapp-theme'

function loadStoredTheme(): ThemeId {
  const stored = window.localStorage.getItem(THEME_STORAGE_KEY)
  return THEMES.find((theme) => theme.id === stored)?.id ?? DEFAULT_THEME_ID
}

/**
 * Phase 3 "Frontend Functional Parity" page: a single local user controls
 * both players (same as the desktop PySide6 GUI today) — no multi-session /
 * auth yet (Phase 2, deliberately deferred).
 */
function App() {
  const [sessionId, setSessionId] = useState<string | null>(null)
  const [gameState, setGameState] = useState<GameState>(initialGameState)
  const [starterMode, setStarterMode] = useState('fixed_x')
  const [themeId, setThemeId] = useState<ThemeId>(loadStoredTheme)
  const disconnectRef = useRef<(() => void) | null>(null)

  useEffect(() => {
    return () => disconnectRef.current?.()
  }, [])

  useEffect(() => {
    window.localStorage.setItem(THEME_STORAGE_KEY, themeId)
  }, [themeId])

  async function handleNewGame(): Promise<void> {
    setGameState((previous) => requestNewGame(previous, starterMode))
    try {
      if (sessionId === null) {
        const session = await createSession(starterMode)
        disconnectRef.current?.()
        setSessionId(session.session_id)
        disconnectRef.current = connectSessionEvents(session.session_id, (envelope) => {
          setGameState((previous) => applyEnvelope(previous, envelope))
        })
      } else {
        // Restarts the match on the same engine connection, without
        // recreating the session/process (mirrors the desktop Reload button).
        await sendCommand(sessionId, CMD_NEW_GAME, { starter_mode: starterMode })
      }
    } catch (caught) {
      setGameState((previous) => ({ ...previous, errorMessage: String(caught) }))
    }
  }

  async function handleCellClick(row: number, col: number): Promise<void> {
    if (sessionId === null || gameState.gameOver || gameState.activeMark === null) {
      setGameState((previous) => ({
        ...previous,
        errorMessage: 'Nessuna partita in corso: premi «Nuova Partita».',
      }))
      return
    }
    try {
      await sendCommand(sessionId, CMD_MOVE, { player: gameState.activeMark, row, col })
    } catch (caught) {
      setGameState((previous) => ({ ...previous, errorMessage: String(caught) }))
    }
  }

  const theme = getTheme(themeId)
  const boardDisabled = sessionId === null || gameState.gameOver || gameState.activeMark === null

  return (
    <div className="app" style={themeToCssVars(theme) as CSSProperties}>
      <header className="app-header">
        <h1>Tic-Tac-Toe — WebApp</h1>
        <GameToolbar
          starterMode={starterMode}
          onStarterModeChange={setStarterMode}
          onNewGame={() => {
            void handleNewGame()
          }}
          themeId={themeId}
          onThemeChange={setThemeId}
        />
      </header>

      <TurnHeader status={gameState.header} />

      <div className="game-layout">
        <TrisBoard
          cells={gameState.cells}
          winLine={gameState.winLine}
          disabled={boardDisabled}
          onCellClick={(row, col) => {
            void handleCellClick(row, col)
          }}
        />
        <MatchLog entries={gameState.logEntries} />
      </div>

      <PlayerBadges playerStatuses={gameState.playerStatuses} />
      <ErrorBar message={gameState.errorMessage} />
    </div>
  )
}

export default App
