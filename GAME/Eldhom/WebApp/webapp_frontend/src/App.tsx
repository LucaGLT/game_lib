import { useEffect, useRef, useState, type CSSProperties } from 'react'
import { createSession, sendCommand } from '@webgui/session/restClient'
import { connectSessionEvents } from '@webgui/session/wsClient'
import { EnvelopeRouter } from '@webgui/session/EnvelopeRouter'
import { useGmGuiModule } from '@webgui/modules/useGmGuiModule'
import { DEFAULT_THEME_ID, THEMES, getTheme, themeToCssVars, type ThemeId } from '@webgui/theme/themes'
import { ErrorBar } from '@webgui/components/ErrorBar'
import { EventLog } from '@webgui/components/EventLog'
import { ThemeSelect } from '@webgui/components/ThemeSelect'
import '@webgui/styles.css'
import { CMD_SIMPLE_ACTION, listMissions, type MissionSummary } from './engine/contract'
import './App.css'

const THEME_STORAGE_KEY = 'eldhom-webapp-theme'

function loadStoredTheme(): ThemeId {
  const stored = window.localStorage.getItem(THEME_STORAGE_KEY)
  return THEMES.find((theme) => theme.id === stored)?.id ?? DEFAULT_THEME_ID
}

/**
 * Phase 1 "Interfaces & Stubs" page: shows the raw JSON of every event
 * received via WS, plus a minimal form to start a mission and send one
 * eldhom.simple_action — enough to validate the full browser <-> eng_serve
 * <-> eldhom_engine.exe round trip. Real map/hand/timeline components come
 * in Phase 3 ("Frontend Functional Parity"), mirroring the same phased
 * approach already used for Tic-Tac-Toe.
 *
 * Generic building blocks (theme, session REST/WS client, EnvelopeRouter,
 * ErrorBar/EventLog/ThemeSelect) come from `webLib/WebGUI_Lib` (`@webgui/*`)
 * — consumed from day one here (unlike Tris, which only adopted it in a
 * later refactor). Everything imported from `./engine` stays Eldhôm-specific.
 */
function App() {
  const [missions, setMissions] = useState<MissionSummary[]>([])
  const [missionId, setMissionId] = useState<string>('')
  const [sessionId, setSessionId] = useState<string | null>(null)
  const [router, setRouter] = useState<EnvelopeRouter | null>(null)
  const [logEntries, setLogEntries] = useState<string[]>([])
  const [errorMessage, setErrorMessage] = useState<string | null>(null)
  const [heroId, setHeroId] = useState('thael')
  const [actionType, setActionType] = useState('MOVE')
  const [destination, setDestination] = useState('S1')
  const [themeId, setThemeId] = useState<ThemeId>(loadStoredTheme)
  const disconnectRef = useRef<(() => void) | null>(null)

  useEffect(() => {
    return () => disconnectRef.current?.()
  }, [])

  useEffect(() => {
    window.localStorage.setItem(THEME_STORAGE_KEY, themeId)
  }, [themeId])

  useEffect(() => {
    listMissions()
      .then((found) => {
        setMissions(found)
        if (found.length > 0) {
          setMissionId(found[0].mission_id)
        }
      })
      .catch((caught) => setErrorMessage(String(caught)))
  }, [])

  // Wildcard subscription: Phase 1 just dumps every envelope as raw JSON, it
  // does not interpret typeIds yet (that reducer work is Phase 3's job).
  useGmGuiModule(router, { subscribedTypeIds: ['*'] }, (envelope) => {
    setLogEntries((previous) => [...previous, JSON.stringify(envelope)])
  })

  async function handleStartMission(): Promise<void> {
    if (missionId === '') {
      setErrorMessage('Seleziona una missione.')
      return
    }
    try {
      const session = await createSession({ mission_id: missionId })
      disconnectRef.current?.()
      const nextRouter = new EnvelopeRouter()
      setRouter(nextRouter)
      setSessionId(session.session_id)
      setLogEntries([])
      setErrorMessage(null)
      disconnectRef.current = connectSessionEvents(session.session_id, (envelope) => {
        nextRouter.dispatch(envelope)
      })
    } catch (caught) {
      setErrorMessage(String(caught))
    }
  }

  async function handleSendAction(): Promise<void> {
    if (sessionId === null) {
      setErrorMessage('Nessuna sessione attiva: avvia prima una missione.')
      return
    }
    try {
      await sendCommand(sessionId, CMD_SIMPLE_ACTION, {
        hero_id: heroId,
        action_type: actionType,
        destination,
      })
    } catch (caught) {
      setErrorMessage(String(caught))
    }
  }

  const theme = getTheme(themeId)

  return (
    <div className="app" style={themeToCssVars(theme) as CSSProperties}>
      <header className="app-header">
        <h1>Eldhôm — WebApp (Phase 1 stub)</h1>
        <ThemeSelect themeId={themeId} onThemeChange={setThemeId} />
      </header>

      <section className="controls">
        <label className="gmgui-field">
          Missione
          <select value={missionId} onChange={(event) => setMissionId(event.target.value)}>
            {missions.map((mission) => (
              <option key={mission.mission_id} value={mission.mission_id}>
                {mission.title}
              </option>
            ))}
          </select>
        </label>
        <button type="button" onClick={() => void handleStartMission()}>
          Avvia missione
        </button>
      </section>

      <section className="controls">
        <label className="gmgui-field">
          Eroe
          <input value={heroId} onChange={(event) => setHeroId(event.target.value)} />
        </label>
        <label className="gmgui-field">
          Azione
          <select value={actionType} onChange={(event) => setActionType(event.target.value)}>
            <option value="MOVE">MOVE</option>
            <option value="ATTACK">ATTACK</option>
            <option value="INTERACT">INTERACT</option>
            <option value="RECOVER">RECOVER</option>
          </select>
        </label>
        <label className="gmgui-field">
          Destinazione
          <input value={destination} onChange={(event) => setDestination(event.target.value)} />
        </label>
        <button type="button" onClick={() => void handleSendAction()}>
          Invia azione semplice
        </button>
      </section>

      <EventLog entries={logEntries} ariaLabel="Log eventi grezzi (JSON)" />
      <ErrorBar message={errorMessage} />
    </div>
  )
}

export default App
