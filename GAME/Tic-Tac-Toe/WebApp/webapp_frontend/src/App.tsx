import { useEffect, useRef, useState, type CSSProperties } from 'react'
import { useAuth } from '@webgui/session/AuthProvider'
import { LoginForm } from '@webgui/components/LoginForm'
import { JoinSessionForm } from '@webgui/components/JoinSessionForm'
import {
  createSession,
  listSessions,
  closeSession,
  sendCommand,
  joinSession,
  getSession,
} from '@webgui/session/restClient'
import { connectSessionEvents } from '@webgui/session/wsClient'
import { EnvelopeRouter } from '@webgui/session/EnvelopeRouter'
import { useGmGuiModule } from '@webgui/modules/useGmGuiModule'
import { DEFAULT_THEME_ID, THEMES, getTheme, themeToCssVars, type ThemeId } from '@webgui/theme/themes'
import { ErrorBar } from '@webgui/components/ErrorBar'
import { EventLog } from '@webgui/components/EventLog'
import { ActorStatusBadges } from '@webgui/components/ActorStatusBadges'
import type { SessionInfo } from '@webgui/session/types'
import '@webgui/styles.css'
import { CMD_MOVE, CMD_NEW_GAME, PLAYER_ACTORS, resolveTrisBadge } from './engine/contract'
import { applyEnvelope, initialGameState, requestNewGame, type GameState } from './engine/gameState'
import GameToolbar from './components/GameToolbar'
import TurnHeader from './components/TurnHeader'
import TrisBoard from './components/TrisBoard'
import './App.css'

const THEME_STORAGE_KEY = 'tris-webapp-theme'

function loadStoredTheme(): ThemeId {
  const stored = window.localStorage.getItem(THEME_STORAGE_KEY)
  return THEMES.find((theme) => theme.id === stored)?.id ?? DEFAULT_THEME_ID
}

function friendlyErrorMessage(caught: unknown): string {
  const message = caught instanceof Error ? caught.message : String(caught)
  if (message.includes('429')) {
    return 'Hai raggiunto il numero massimo di sessioni contemporanee: chiudine una per continuare.'
  }
  if (message.includes('joinSession') && message.includes('404')) {
    return 'Codice partita non valido.'
  }
  if (message.includes('joinSession') && message.includes('409')) {
    return 'Questa partita ha già due giocatori.'
  }
  return message
}

/**
 * Phase 3 "Frontend Functional Parity" page, extended in Phase 2 with a
 * pilot-grade login gate + per-user session picker (Multi-Session & Auth,
 * built library-level in `pyLib/gmWebServe` + `webLib/WebGUI_Lib`): each
 * authenticated user controls their own match(es), up to the server-side
 * concurrent-session cap (see `eng_serve/auth_config.json`).
 *
 * Generic building blocks (auth, theme, session REST/WS client,
 * EnvelopeRouter, ErrorBar/EventLog/ActorStatusBadges) come from
 * `webLib/WebGUI_Lib` (`@webgui/*`) — the web equivalent of `pyLib/gmGui`.
 * Everything imported from `./engine` and `./components` stays Tris-specific.
 */
function App() {
  const auth = useAuth()
  const [sessionId, setSessionId] = useState<string | null>(null)
  const [activeSession, setActiveSession] = useState<SessionInfo | null>(null)
  const [sessions, setSessions] = useState<SessionInfo[]>([])
  const [router, setRouter] = useState<EnvelopeRouter | null>(null)
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

  useEffect(() => {
    if (auth.session === null) {
      setSessions([])
      return
    }
    listSessions(auth.session.token)
      .then(setSessions)
      .catch((caught) => {
        setGameState((previous) => ({ ...previous, errorMessage: friendlyErrorMessage(caught) }))
      })
  }, [auth.session])

  const myRole = activeSession?.your_role ?? null
  const needsOpponent =
    activeSession !== null && Object.values(activeSession.roles).some((holder) => holder === null)
  const authToken = auth.session?.token ?? null

  // While waiting for a second (different) user to join via the join code,
  // poll their seat every few seconds — the engine has no "someone joined"
  // event of its own (joining is a pure eng_serve/session concept), so this
  // is the only way the creator finds out the opponent is in without a
  // manual page reload.
  useEffect(() => {
    if (authToken === null || sessionId === null || !needsOpponent) {
      return
    }
    const intervalId = window.setInterval(() => {
      getSession(authToken, sessionId)
        .then(setActiveSession)
        .catch(() => {
          // Best-effort refresh only; a transient failure here is not fatal.
        })
    }, 3000)
    return () => window.clearInterval(intervalId)
  }, [authToken, sessionId, needsOpponent])

  // The Tris reducer subscribes to every typeId ("*") via the shared router —
  // it is itself just one more `EnvelopeRouter` consumer, on equal footing
  // with the generic `ActorStatusBadges` module below (which independently
  // subscribes to only the `gmActor.*` typeIds it needs).
  useGmGuiModule(router, { subscribedTypeIds: ['*'] }, (envelope) => {
    setGameState((previous) => applyEnvelope(previous, envelope))
  })

  function connectToSession(token: string, session: SessionInfo): void {
    disconnectRef.current?.()
    const nextRouter = new EnvelopeRouter()
    setRouter(nextRouter)
    setSessionId(session.session_id)
    setActiveSession(session)
    disconnectRef.current = connectSessionEvents(token, session.session_id, (envelope) => {
      nextRouter.dispatch(envelope)
    })
  }

  async function handleNewGame(): Promise<void> {
    if (auth.session === null) {
      return
    }
    const token = auth.session.token
    setGameState((previous) => requestNewGame(previous, starterMode))
    try {
      if (sessionId === null) {
        const session = await createSession(token, { starter_mode: starterMode })
        setSessions((previous) => [...previous, session])
        connectToSession(token, session)
      } else {
        // Restarts the match on the same engine connection, without
        // recreating the session/process (mirrors the desktop Reload button).
        await sendCommand(token, sessionId, CMD_NEW_GAME, { starter_mode: starterMode })
      }
    } catch (caught) {
      setGameState((previous) => ({ ...previous, errorMessage: friendlyErrorMessage(caught) }))
    }
  }

  async function handleCellClick(row: number, col: number): Promise<void> {
    if (auth.session === null || sessionId === null || gameState.gameOver || gameState.activeMark === null) {
      setGameState((previous) => ({
        ...previous,
        errorMessage: 'Nessuna partita in corso: premi «Nuova Partita».',
      }))
      return
    }
    if (gameState.activeMark !== myRole) {
      setGameState((previous) => ({ ...previous, errorMessage: 'Non è il tuo turno.' }))
      return
    }
    try {
      // `player` is only a hint: the server always re-derives the real mark
      // from the caller's own seat, so this can never move on the opponent's
      // behalf even if this check above were somehow bypassed client-side.
      await sendCommand(auth.session.token, sessionId, CMD_MOVE, { player: gameState.activeMark, row, col })
    } catch (caught) {
      setGameState((previous) => ({ ...previous, errorMessage: friendlyErrorMessage(caught) }))
    }
  }

  function handleResumeSession(id: string): void {
    if (auth.session === null) {
      return
    }
    const session = sessions.find((candidate) => candidate.session_id === id)
    if (session === undefined) {
      return
    }
    connectToSession(auth.session.token, session)
  }

  async function handleJoinSession(joinCode: string): Promise<void> {
    if (auth.session === null) {
      return
    }
    const token = auth.session.token
    let session: SessionInfo
    try {
      session = await joinSession(token, joinCode)
    } catch (caught) {
      throw new Error(friendlyErrorMessage(caught))
    }
    setSessions((previous) => {
      const exists = previous.some((candidate) => candidate.session_id === session.session_id)
      return exists
        ? previous.map((candidate) => (candidate.session_id === session.session_id ? session : candidate))
        : [...previous, session]
    })
    connectToSession(token, session)
  }

  async function handleCloseSession(id: string): Promise<void> {
    if (auth.session === null) {
      return
    }
    try {
      await closeSession(auth.session.token, id)
    } catch (caught) {
      setGameState((previous) => ({ ...previous, errorMessage: friendlyErrorMessage(caught) }))
      return
    }
    setSessions((previous) => previous.filter((session) => session.session_id !== id))
    if (sessionId === id) {
      disconnectRef.current?.()
      disconnectRef.current = null
      setSessionId(null)
      setActiveSession(null)
      setRouter(null)
      setGameState(initialGameState)
    }
  }

  function handleLogout(): void {
    disconnectRef.current?.()
    disconnectRef.current = null
    setSessionId(null)
    setActiveSession(null)
    setRouter(null)
    setGameState(initialGameState)
    setSessions([])
    auth.logout()
  }

  const theme = getTheme(themeId)

  if (auth.isRestoring) {
    return (
      <div className="app gmgui-theme-backdrop" data-theme={themeId} style={themeToCssVars(theme) as CSSProperties}>
        <p>Verifica sessione in corso…</p>
      </div>
    )
  }

  if (!auth.isAuthenticated) {
    return (
      <div className="app gmgui-theme-backdrop" data-theme={themeId} style={themeToCssVars(theme) as CSSProperties}>
        <LoginForm
          title="Tic-Tac-Toe — Accedi"
          onSubmit={(username, password) => auth.login(username, password)}
        />
      </div>
    )
  }

  const boardDisabled =
    sessionId === null ||
    gameState.gameOver ||
    gameState.activeMark === null ||
    gameState.activeMark !== myRole

  return (
    <div className="app gmgui-theme-backdrop" data-theme={themeId} style={themeToCssVars(theme) as CSSProperties}>
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
        <div className="app-header__account">
          <span>{auth.session?.username}</span>
          {sessionId !== null && (
            <button type="button" onClick={() => void handleCloseSession(sessionId)}>
              Chiudi sessione
            </button>
          )}
          <button type="button" onClick={handleLogout}>
            Esci
          </button>
        </div>
      </header>

      {sessionId === null ? (
        <div className="gmgui-session-picker">
          <h2>Sessioni attive</h2>
          {sessions.length === 0 && (
            <p>Nessuna sessione attiva. Premi «Nuova Partita» per iniziare, oppure entra in una partita con un codice.</p>
          )}
          {sessions.map((session) => (
            <div key={session.session_id} className="gmgui-session-picker__row">
              <span>
                Sessione {session.session_id.slice(0, 8)} ·{' '}
                <span className="gmgui-session-picker__code">{session.join_code}</span>
                {' · '}
                {Object.entries(session.roles)
                  .map(([role, username]) => `${role}: ${username ?? 'in attesa'}`)
                  .join(' · ')}
              </span>
              <div className="gmgui-session-picker__row-actions">
                <button type="button" onClick={() => handleResumeSession(session.session_id)}>
                  Riprendi
                </button>
                <button type="button" onClick={() => void handleCloseSession(session.session_id)}>
                  Chiudi
                </button>
              </div>
            </div>
          ))}
          <JoinSessionForm onSubmit={handleJoinSession} />
        </div>
      ) : (
        <>
          {myRole !== null && (
            <p className="tris-role-banner">
              Sei il Giocatore <strong>{myRole}</strong>
              {needsOpponent && activeSession !== null && (
                <>
                  {' — in attesa dell\'avversario. Condividi il codice: '}
                  <span className="gmgui-session-picker__code">{activeSession.join_code}</span>
                </>
              )}
            </p>
          )}
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
            <EventLog entries={gameState.logEntries} ariaLabel="Log della partita" />
          </div>

          <ActorStatusBadges router={router} actors={PLAYER_ACTORS} resolveBadge={resolveTrisBadge} />
        </>
      )}

      <ErrorBar message={gameState.errorMessage} />
    </div>
  )
}

export default App
